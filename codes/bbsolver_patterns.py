# bbsolver_patterns.py

# Standard Library Imports
import time
from copy import deepcopy
import cProfile

# Third-Party Imports
import numpy as np
import pandas as pd
from wntr.network import WaterNetworkModel
from wntr.sim import EpanetSimulator
from wntr.sim.results import SimulationResults
from rich import print


# Local Application/Library Imports
from economic_custom import pump_cost

from typing import Dict, List

from rich.console import Console
from rich.table import Table


class BBCounter:
    def __init__(self, hmax: int, num_pumps: int):
        self.num_pumps = num_pumps
        self.h = 0
        self.hmax = hmax
        self.y = np.zeros(hmax + 1, dtype=int)

    def update_y(self, is_feasible: bool) -> bool:
        if self.h == 0 and not is_feasible:
            return False

        if is_feasible and self.h < len(self.y) - 1:
            # reset the counter for the next hour
            self.h += 1
            self.y[self.h] = 0
            return True

        if self.y[self.h] == self.num_pumps:
            self.y[self.h] = 0
            self.h -= 1
            return self.update_y(False)

        if self.y[self.h] < self.num_pumps:
            self.y[self.h] += 1
            return True

        return False

    def jump_to_end(self, h: int):
        self.y[h] = self.num_pumps


def sim_run(wn: WaterNetworkModel, h: int, verbose: bool = False):
    wn.options.time.duration = 3600 * h  # seconds
    sim = EpanetSimulator(wn)
    out = sim.run_sim()
    return out


def show_x(x: np.ndarray, h: int):
    for i in range(h):
        print(f"x[{i+1:2d}]: {x[i+1, :]}")


def calc_actuations_csum(x: np.ndarray, h: int):
    xh = x[:h, :]
    dx = np.diff(xh, axis=0)
    actuations = np.sum(dx > 0, axis=0)
    return actuations


def update_x(x: np.ndarray, y: np.ndarray, h: int, max_actuations: int, verbose: bool = False) -> bool:
    """
    Update the pump activation matrix x based on the current state y and hour h.

    Parameters:
    - x: Activation matrix (hours x pumps)
    - y: Current number of actuations required per hour
    - h: Current hour
    - max_actuations: Maximum allowed actuations per pump
    - verbose: If True, print debug statements

    Returns:
    - True if the update is successful and feasible
    - False otherwise
    """
    y_old = y[h - 1]
    y_new = y[h]
    x_old = x[h - 1]
    x_new = x[h]

    # Start by copying the previous state
    x_new[:] = x_old[:]

    if y_new == y_old:
        if verbose:
            print(f"Hour {h}: No change in actuations required (y_new={y_new} == y_old={y_old}).")
        return True

    # Calculate the cumulative actuations up to hour h
    actuations_csum = calc_actuations_csum(x, h)

    # Sort pumps by the lowest number of actuations to prioritize less-used pumps
    pumps_sorted = np.argsort(actuations_csum)

    if y_new > y_old:
        num_actuations = y_new - y_old
        # Identify pumps that are not currently actuating
        pumps_to_actuate = [pump for pump in pumps_sorted if x_old[pump] == 0]
        # Select the first 'num_actuations' pumps with the fewest actuations
        pumps_to_actuate = pumps_to_actuate[:num_actuations]

        if verbose:
            print(f"Hour {h}: Need to actuate {num_actuations} pump(s). Pumps selected: {pumps_to_actuate}")

        # Check if actuating these pumps would exceed max_actuations
        if any(actuations_csum[pump] >= max_actuations for pump in pumps_to_actuate):
            if verbose:
                print(f"Hour {h}: Actuating pumps {pumps_to_actuate} would exceed max_actuations ({max_actuations}).")
            return False

        # Actuate the selected pumps
        x_new[pumps_to_actuate] = 1

        if verbose:
            print(f"Hour {h}: Updated x_new after actuating: {x_new}")

        return True

    elif y_new < y_old:
        num_deactuations = y_old - y_new
        # Identify pumps that are currently actuating
        pumps_actuating = [pump for pump in pumps_sorted if x_old[pump] == 1]
        # Select the first 'num_deactuations' pumps to deactuate
        pumps_to_deactuate = pumps_actuating[:num_deactuations]

        if verbose:
            print(f"Hour {h}: Need to deactuate {num_deactuations} pump(s). Pumps selected: {pumps_to_deactuate}")

        # Deactuate the selected pumps
        x_new[pumps_to_deactuate] = 0

        if verbose:
            print(f"Hour {h}: Updated x_new after deactuating: {x_new}")

        return True

    else:
        # This case should not occur due to the initial equality check
        if verbose:
            print(f"Hour {h}: Unexpected condition encountered.")
        return False


def update_pumps(wn: WaterNetworkModel, h: int, x: np.ndarray, verbose: bool):
    pumps: List[str] = ["PMP111", "PMP222", "PMP333"]
    multipliers: Dict[str, np.ndarray] = {pump: wn.get_pattern(pump).multipliers for pump in pumps}
    for pump_id, pump_name in enumerate(pumps):
        pump_mult = multipliers[pump_name]
        pump_mult[:h] = x[1 : h + 1, pump_id]
        if verbose:
            print(f"   PUMP={pump_name}")
            print(f"   MULT={pump_mult[:h]}")


def check_pressures(out: SimulationResults, h: int, verbose: bool = False) -> bool:
    """
    Check if the pressures at specified nodes meet the minimum required thresholds.

    Parameters:
    - out: SimulationResults instance containing simulation outputs.
    - h: Current hour in the simulation.
    - verbose: If True, print detailed pressure checks with styled messages.

    Returns:
    - True if all pressures meet or exceed their thresholds.
    - False if any pressure is below its threshold.
    """
    # Define pressure thresholds for each node
    pressure_thresholds: Dict[str, float] = {"55": 42, "90": 51, "170": 30}

    # Get pressures
    pressures = out.node["pressure"]

    # Calculate the simulation step corresponding to the current hour
    simulation_step = 3600 * h

    # Flag to track overall pressure feasibility
    pressures_ok = True

    # Initialize a list to store messages for each node
    message: List[List[str]] = []

    # Define pressure condition: pressure >= threshold
    if verbose:
        print(f"\n[bold blue]Checking Pressures at Hour {h}[/bold blue]")

    for node_name, threshold in pressure_thresholds.items():
        # Retrieve the pressure value for the current node and simulation step
        try:
            pressure = pressures.loc[simulation_step, node_name]
        except KeyError:
            if verbose:
                message.append(["❌", node_name, f"Node '{node_name}' not found in simulation results."])
            pressures_ok = False
            continue

        # Check if the pressure meets the threshold
        if pressure < threshold:
            pressures_ok = False
            if verbose:
                message.append(
                    [
                        "⚠️",
                        node_name,
                        f"[bold yellow]has pressure {pressure:.2f} <  {threshold}[/bold yellow]",
                    ]
                )
        else:
            if verbose:
                message.append(
                    [
                        "✅",
                        node_name,
                        f"[bold green]has pressure {pressure:.2f} >= {threshold}[/bold green]",
                    ]
                )

    if verbose:
        # Create a pandas DataFrame from the messages
        df = pd.DataFrame(message, columns=["Status", "Node", "Message"])

        # Initialize Rich console
        console = Console()

        # Create a Rich Table
        table = Table(show_header=True, header_style="bold magenta")
        table.add_column("Status", style="dim", width=6)
        table.add_column("Node", style="dim", width=10)
        table.add_column("Message", style="dim")

        # Populate the table with DataFrame rows
        for _, row in df.iterrows():
            table.add_row(row["Status"], row["Node"], row["Message"])

        # Print the table to the console
        console.print(table)

        # Final summary message
        if pressures_ok:
            print("[bold green]All node pressures are within acceptable limits.[/bold green]\n")
        else:
            print("[bold red]Some node pressures are below the required thresholds.[/bold red]\n")

    return pressures_ok


def check_levels(out: SimulationResults, h: int, verbose: bool = False) -> bool:
    """
    Check if the water levels in specified tanks are within the acceptable range.

    Parameters:
    - out: SimulationResults instance containing simulation outputs.
    - h: Current hour in the simulation.
    - verbose: If True, print detailed level checks with styled messages.

    Returns:
    - True if all tank levels are within the acceptable range.
    - False if any tank level is outside the acceptable range or if a tank is missing.
    """
    # List of tanks to monitor
    tanks: List[str] = ["65", "165", "265"]

    # Get water levels (heads) from simulation results
    levels = out.node["head"]

    # Calculate the simulation step corresponding to the current hour
    simulation_step = 3600 * h

    # Flag to track overall level feasibility
    levels_ok = True

    # Define level thresholds
    level_min: float = 66.53
    level_max: float = 71.53

    # Initialize a list to store messages for each tank
    message: List[List[str]] = []

    # Define level condition: level_min <= level <= level_max
    if verbose:
        print(f"\n[bold blue]Checking Levels at Hour {h}[/bold blue]")

    for tank_name in tanks:
        # Retrieve the level value for the current tank and simulation step
        try:
            level = levels.loc[simulation_step, tank_name]
        except KeyError:
            if verbose:
                message.append(["❌", tank_name, f"Tank '{tank_name}' not found in simulation results."])
            levels_ok = False
            # Append error message to the list
            continue

        # Check if the level meets the threshold
        if level < level_min or level > level_max:
            levels_ok = False
            if verbose:
                message.append(
                    [
                        "⚠️",
                        tank_name,
                        f"[bold yellow]has level {level:.3f} not in [{level_min}, {level_max}][/bold yellow]",
                    ]
                )
        else:
            if verbose:
                message.append(
                    [
                        "✅",
                        tank_name,
                        f"[bold green]has level {level:.2f} within [{level_min}, {level_max}][/bold green]",
                    ]
                )

    if verbose:
        # Create a pandas DataFrame from the messages
        df = pd.DataFrame(message, columns=["Status", "Tank", "Message"])

        # Initialize Rich console
        console = Console()

        # Create a Rich Table
        table = Table(show_header=True, header_style="bold magenta")
        table.add_column("Status", style="dim", width=6)
        table.add_column("Tank", style="dim", width=10)
        table.add_column("Message", style="dim")

        # Populate the table with DataFrame rows
        for _, row in df.iterrows():
            table.add_row(row["Status"], row["Tank"], row["Message"])

        # Print the table to the console
        console.print(table)

        # Final summary message
        if levels_ok:
            print("[bold green]All tank levels are within acceptable limits.[/bold green]\n")
        else:
            print("[bold red]Some tank levels are outside the required thresholds.[/bold red]\n")

    return levels_ok


def check_stability(out: SimulationResults, h: int, verbose: bool = False) -> bool:
    """
    Check if the water levels in specified tanks remain stable throughout the simulation.

    Stability is determined by ensuring that the final level of each tank is not below its initial level.

    Parameters:
    - out: SimulationResults instance containing simulation outputs.
    - h: Current hour in the simulation.
    - verbose: If True, print detailed stability checks with styled messages.

    Returns:
    - True if all tank levels are stable (final level >= initial level).
    - False if any tank level is unstable or if a tank is missing.
    """
    # List of tanks to monitor
    tanks: List[str] = ["65", "165", "265"]

    # Get water levels (heads) from simulation results
    levels = out.node["head"]

    # Calculate the simulation step corresponding to the current hour
    simulation_step = 3600 * h

    # Flag to track overall stability feasibility
    stability_ok = True

    # Initialize a list to store messages for each tank
    message: List[List[str]] = []

    # Define stability condition: final level >= initial level
    if verbose:
        print(f"\n[bold blue]Checking Stability at Hour {h}[/bold blue]")

    for tank_name in tanks:
        # Retrieve the initial and final level values for the current tank and simulation step
        try:
            # Initial level at simulation step 0
            level_initial = levels.loc[0, tank_name]
            # Final level at the current simulation step
            level_final = levels.loc[simulation_step, tank_name]
        except KeyError:
            if verbose:
                message.append(["❌", tank_name, f"Tank '{tank_name}' not found in simulation results."])
            stability_ok = False
            continue

        # Check if the final level meets the stability condition
        if level_final < level_initial:
            stability_ok = False
            if verbose:
                message.append(
                    [
                        "⚠️",
                        tank_name,
                        f"[bold yellow]has final level {level_final:.3f} < initial level {level_initial:.2f}[/bold yellow]",
                    ]
                )
        else:
            # Append success message to the list
            if verbose:
                message.append(
                    [
                        "✅",
                        tank_name,
                        f"[bold green]has final level {level_final:.3f} ≥ initial level {level_initial:.2f}[/bold green]",
                    ]
                )

    if verbose:
        # Create a pandas DataFrame from the messages
        df = pd.DataFrame(message, columns=["Status", "Tank", "Message"])

        # Initialize Rich console
        console = Console()

        # Create a Rich Table
        table = Table(show_header=True, header_style="bold magenta")
        table.add_column("Status", style="dim", width=6)
        table.add_column("Tank", style="dim", width=10)
        table.add_column("Message", style="dim")

        # Populate the table with DataFrame rows
        for _, row in df.iterrows():
            table.add_row(row["Status"], row["Tank"], row["Message"])

        # Print the table to the console
        console.print(table)

        # Final summary message
        if stability_ok:
            print("[bold green]All tank levels are stable within acceptable limits.[/bold green]\n")
        else:
            print("[bold red]Some tank levels are unstable and do not meet the required thresholds.[/bold red]\n")

    return stability_ok


class BBStatistics:
    def __init__(self, hmax):
        self.hmax = hmax
        # Track pruning counts per hour for each reason
        self.prune_keys = ["actuations", "cost", "pressure", "level", "stability"]
        self.prune_counts_by_hour = {h: {reason: 0 for reason in self.prune_keys} for h in range(hmax + 1)}
        # Track feasible/infeasible solutions per hour
        self.solutions_per_hour = {h: {"feasible": 0, "infeasible": 0} for h in range(hmax + 1)}

    def record_pruning(self, reason: str, h: int):
        """Record a pruning event for both overall counts and hour-specific counts."""
        if reason in self.prune_keys:
            self.prune_counts_by_hour[h][reason] += 1
            self.record_solution(h, False)
        else:
            raise ValueError(f"Invalid pruning reason: {reason}")

    def record_solution(self, h, is_feasible):
        """Record whether a solution at hour h was feasible."""
        key = "feasible" if is_feasible else "infeasible"
        self.solutions_per_hour[h][key] += 1

    def get_stats(self):
        df = {col: [] for col in self.prune_keys}
        df["infeasible"] = []
        df["feasible"] = []
        for h in range(1, self.hmax + 1):
            df["feasible"].append(self.solutions_per_hour[h]["feasible"])
            df["infeasible"].append(self.solutions_per_hour[h]["infeasible"])
            for reason in self.prune_keys:
                df[reason].append(self.prune_counts_by_hour[h][reason])
        df = pd.DataFrame(df, index=pd.Index(range(1, self.hmax + 1), name="hour"))
        df["total"] = df["feasible"] + df["infeasible"]
        # Convert all columns, but total, to %
        for col in [col for col in df.columns if col != "total"]:
            df[col] = 100 * df[col] / df["total"]
        # Round to 2 decimal places
        df = df.round(1)
        return df

    def print_summary(self):
        """Print a comprehensive statistical summary using pandas and rich."""
        console = Console()

        df = self.get_stats()

        # Table with pruning reasons per hour
        # Create a rich table
        table = Table(title="Pruning Statistics by Hour")

        # Add columns
        table.add_column("Hour", justify="right", style="cyan")
        for column in df.columns:
            if column == "total":
                style = "bold dim"
            elif column == "feasible":
                style = "bold green"
            else:
                style = "bold red"
            table.add_column(column, justify="right", style=style)

        # Add rows
        for idx, row in df.iterrows():
            table.add_row(str(idx), *[str(val) for val in row])

        console.print(table)


def bbsolver(hmax: int = 24, max_actuations: int = 3, verbose: bool = False):
    fn_inp = "networks/any-town.inp"
    wn = WaterNetworkModel(fn_inp)
    verbose = False

    # Show config
    print(f"Network: {fn_inp}")
    print(f"Max actuations: {max_actuations}")
    print(f"Last hour: {hmax}")
    print(f"Verbose: {verbose}\n")

    # Initialize x and counter
    x = np.zeros((hmax + 1, 3), dtype=int)
    counter = BBCounter(hmax, max_actuations)

    is_feasible = True
    niter = 0
    cost_min = np.inf

    # Add timer
    start_time = time.time()

    # Initialize statistics tracker
    stats = BBStatistics(hmax)

    while counter.update_y(is_feasible):
        niter += 1
        elapsed_time = time.time() - start_time
        avg_time_per_iter = elapsed_time / niter
        print(
            f"[bold blue]⏱  Iter: {niter} | Time: {elapsed_time:.2f}s | Avg: {avg_time_per_iter:.4f}s[/bold blue]",
            end="\r",
        )

        y, h = counter.y, counter.h
        is_feasible = update_x(x, y, h, max_actuations, verbose=False)

        if not is_feasible:
            stats.record_pruning("actuations", h)
            if verbose:
                print("[bold yellow]⚠️  Pruned: max_actuations[/bold yellow]")
            continue

        # Check that the number of actuations matches the number of 1s in y
        assert y[h] == sum(x[h]), f"y={y[h]} != sum(x)={sum(x[h])}"

        # Display iteration details using Rich (if verbose)
        if verbose:
            print(f"\n\nh[{h:2d}], y{y[1:h+1]}")
            show_x(x, h)

        # Update pump statuses without verbosity
        update_pumps(wn, h, x, verbose=False)

        # Run the simulation
        out = sim_run(wn, h, verbose=verbose)
        cost = pump_cost(out, wn)
        if verbose:
            print(f"\ncost={cost:.2f}, cost_min={cost_min:.2f}")

        # Determine feasibility based on cost
        is_feasible = cost < cost_min
        if not is_feasible:
            stats.record_pruning("cost", h)
            if verbose:
                print(f"\n[bold yellow]⚠️  Pruned: cost[/bold yellow]")
            # Can prune the entire level
            counter.jump_to_end(h)
            continue

        # Check pressures
        is_feasible = check_pressures(out, h, verbose=verbose)
        if not is_feasible:
            stats.record_pruning("pressure", h)
            if verbose:
                print("[bold yellow]⚠️  Pruned: pressure[/bold yellow]")
            continue

        # Check levels
        is_feasible = check_levels(out, h, verbose=verbose)
        if not is_feasible:
            stats.record_pruning("level", h)
            if verbose:
                print("[bold yellow]⚠️  Pruned: level[/bold yellow]")
            continue

        # Check stability
        is_feasible = True if h < hmax else check_stability(out, h, verbose=verbose)
        if not is_feasible:
            stats.record_pruning("stability", h)
            if verbose:
                print("[bold yellow]⚠️  Pruned: stability[/bold yellow]")
            continue

        # Record solution feasibility for this hour
        stats.record_solution(h, True)
        if h == hmax:  # last hour
            cost_min = cost
            y_min = deepcopy(y)
            print(f"\n[bold green]💰 [{niter}] cost_min updated: {cost_min:.2f}[/bold green]")
            print(f"y_min={y_min[1:]}")

    # Calculate and print timing details
    end_time = time.time()
    total_time = end_time - start_time
    time_per_iter = total_time / niter if niter > 0 else 0

    print(f"\n\nTiming Details:")
    print(f"Total runtime: {total_time:.2f} seconds")
    print(f"Average time per iteration: {time_per_iter:.4f} seconds")
    print(f"\nCompleted in {niter} iterations with a minimum cost of {cost_min:.2f}.")
    print(f"Optimal solution: y={y_min[1:]}")

    # Print statistics at the end
    stats.print_summary()


def profile_bbsolver():
    profiler = cProfile.Profile()
    profiler.enable()
    bbsolver(hmax=6, max_actuations=3, verbose=False)
    profiler.disable()
    # Dump stats to file
    profiler.dump_stats("bbsolver_profile.stats")


def test_cost():
    # Solution from Cost2015
    y = [1, 2, 1, 2, 1, 1, 1, 1, 0, 0, 2, 2, 2, 2, 2, 1, 2, 1, 0, 0, 0, 2, 1, 0]
    print(f"len(y)={len(y)}, y={y}")

    # Add initial zero to match the expected input
    y = [0] + y
    x = np.zeros((len(y), 3), dtype=int)
    for h in range(1, 25):
        is_feasible = update_x(x, y, h, 3, verbose=False)
        assert y[h] == sum(x[h]), f"y={y[h]} != sum(x)={sum(x[h])}"
        if not is_feasible:
            raise ValueError(f"h={h:2d}, is_feasible={is_feasible}")
    print(f"h={h:2d}, is_feasible={is_feasible}")
    show_x(x, h)

    fn_inp = "networks/any-town.inp"
    wn = WaterNetworkModel(fn_inp)
    out = sim_run(wn, 24, verbose=False)
    cost = pump_cost(out, wn)
    print(f"cost={cost:.2f}")


def main():
    import argparse

    parser = argparse.ArgumentParser(description="Run the B&B solver for the given network.")
    parser.add_argument("--hmax", type=int, default=24, help="Maximum number of hours to simulate.")
    parser.add_argument("--max_actuations", type=int, default=3, help="Maximum number of actuations per hour.")
    parser.add_argument("--verbose", action="store_true", help="Print verbose output.")
    args = parser.parse_args()
    bbsolver(hmax=args.hmax, max_actuations=args.max_actuations, verbose=args.verbose)


if __name__ == "__main__":
    # profile_bbsolver()
    test_cost()
    # main()
