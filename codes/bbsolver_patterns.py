# bbsolver_patterns.py

# Standard Library Imports
from copy import deepcopy

# Third-Party Imports
import numpy as np
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
import wntr
from wntr.network import LinkStatus, WaterNetworkModel
from wntr.sim import EpanetSimulator
from wntr.sim.results import SimulationResults
from rich import print

# Local Application/Library Imports
import economic_custom as ec
from economic_custom import pump_cost

from typing import Dict, List


def sim_run(wn: WaterNetworkModel, h: int, verbose: bool = False):
    wn.options.time.duration = 3600 * h  # seconds
    sim = EpanetSimulator(wn)
    out = sim.run_sim()
    return out


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
    pressure_thresholds: dict = {"55": 42, "90": 51, "170": 30}

    # Get pressures
    pressures = out.node["pressure"]

    # Calculate the simulation step corresponding to the current hour
    simulation_step = 3600 * h

    # Flag to track overall pressure feasibility
    pressures_ok = True

    if verbose:
        print(f"\n[bold blue]Checking Pressures at Hour {h}[/bold blue]")

    for node_name, threshold in pressure_thresholds.items():
        # Retrieve the pressure value for the current node and simulation step
        try:
            pressure = pressures.loc[simulation_step, node_name]
        except KeyError:
            if verbose:
                print(f"[bold red]❌ Error: Node '{node_name}' not found in simulation results.[/bold red]")
            pressures_ok = False
            continue

        # Check if the pressure meets the threshold
        if pressure < threshold:
            pressures_ok = False
            if verbose:
                print(
                    f"[bold red]⚠️  Pressure Alert: Node {node_name} has pressure {pressure:.2f} < {threshold}[/bold red]"
                )
        else:
            if verbose:
                print(
                    f"[bold green]✅ Pressure OK: Node {node_name} has pressure {pressure:.2f} >= {threshold}[/bold green]"
                )

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

    if verbose:
        print(f"\n[bold blue]Checking Levels at Hour {h}[/bold blue]")

    for tank_name in tanks:
        # Retrieve the level value for the current tank and simulation step
        try:
            level = levels.loc[simulation_step, tank_name]
        except KeyError:
            if verbose:
                print(f"[bold red]❌ Error: Tank '{tank_name}' not found in simulation results.[/bold red]")
            levels_ok = False
            continue

        # Check if the level meets the threshold
        if level < level_min or level > level_max:
            levels_ok = False
            if verbose:
                print(
                    f"[bold red]⚠️  Level Alert: Tank {tank_name} has level {level:.3f} not in [{level_min}, {level_max}][/bold red]"
                )
        else:
            if verbose:
                print(
                    f"[bold green]✅ Level OK: Tank {tank_name} has level {level:.2f} within [{level_min}, {level_max}][/bold green]"
                )

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

    # Define stability thresholds (if any additional thresholds are needed)
    # For now, stability is defined as final level >= initial level
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
                print(f"[bold red]❌ Error: Tank '{tank_name}' not found in simulation results.[/bold red]")
            stability_ok = False
            continue

        # Check if the final level meets the stability condition
        if level_final < level_initial:
            stability_ok = False
            if verbose:
                print(
                    f"[bold red]⚠️  Stability Alert: Tank {tank_name} has final level {level_final:.3f} < initial level {level_initial:.3f}[/bold red]"
                )
        else:
            if verbose:
                print(
                    f"[bold green]✅ Stability OK: Tank {tank_name} has final level {level_final:.3f} >= initial level {level_initial:.3f}[/bold green]"
                )

    if verbose:
        if stability_ok:
            print("[bold green]All tank levels are stable within acceptable limits.[/bold green]\n")
        else:
            print("[bold red]Some tank levels are unstable and do not meet the required thresholds.[/bold red]\n")

    return stability_ok


def bbsolver():
    wn = WaterNetworkModel("networks/any-town.inp")
    node_name_list = ["55", "90", "170"]
    max_actuations = 1
    hmax = 3

    x = np.zeros((hmax + 1, 3), dtype=int)
    counter = BBCounter(hmax, max_actuations + 1)

    is_feasible = True
    niter = 0
    cost_min = np.inf
    while counter.update_y(is_feasible):
        niter += 1
        print("-" * 120)
        y, h = counter.y, counter.h
        is_feasible = update_x(x, y, h, max_actuations, verbose=False)

        # Display iteration details using Rich
        print(f"[bold blue]Iteration: {niter}[/bold blue]")
        print(f"h={h}, y{y[1:h+1]}")
        show_x(x, h)

        if not is_feasible:
            # Use a warning icon with styled text for pruning due to max_actuations
            print("[bold yellow]⚠️  Pruned: max_actuations[/bold yellow]")
            continue

        assert y[h] == sum(x[h]), f"y={y[h]} != sum(x)={sum(x[h])}"

        # Update pump statuses without verbosity
        update_pumps(wn, h, x, False)

        out = sim_run(wn, h, verbose=False)
        cost = pump_cost(out, wn)
        print(f"cost={cost:.2f}, cost_min={cost_min:.2f}")

        is_feasible = cost < cost_min
        if not is_feasible:
            print("[bold yellow]⚠️  Pruned: cost[/bold yellow]")
            # Can prune the entire level
            counter.jump_to_end(h)
            continue

        is_feasible = check_pressures(out, h, True)
        if not is_feasible:
            print("[bold yellow]⚠️  Pruned: pressure[/bold yellow]")
            continue

        is_feasible = check_levels(out, h, True)
        if not is_feasible:
            print("[bold yellow]⚠️  Pruned: level[/bold yellow]")
            continue

        if h == hmax:  # last hour
            # Check stability
            is_feasible = check_stability(out, h, verbose=True)
            if not is_feasible:
                print("[bold yellow]⚠️  Pruned: stability[/bold yellow]")
                continue

            cost_min = cost
            print(f"[bold red]💰 cost_min updated: {cost_min:.2f}[/bold red]")


if __name__ == "__main__":
    bbsolver()
