# codes/bbsolver.py

# Standard library imports
import os
from typing import List, Tuple, Dict
from copy import deepcopy

# Third-party imports
import numpy as np
import wntr
from wntr.network import LinkStatus, WaterNetworkModel
from wntr.network.controls import ControlAction, Control, Comparison

# Rich library imports for console output
from rich.console import Console
from rich.table import Table
from rich import box
from rich.text import Text

console = Console()


def load_network(fn: str) -> WaterNetworkModel:
    """
    Load a network from a file.
    """
    if not os.path.exists(fn):
        console.print(f"[red]Network file {fn} not found![/red]")
        exit(1)
    return WaterNetworkModel(fn)


# Define global parameters
WN = load_network("networks/any-town.inp")
PUMPS = WN.pump_name_list
N_PUMPS = WN.num_pumps
TIME_STEP = 60 * 60  # 1 hour (in seconds)
N_STEPS = round(24 * 60 * 60 / TIME_STEP)  # number of time steps in a day
OMEGA = 0.5  # weight for the weighted best-first search
LOWER_BOUND = np.inf  # lower bound for the cost
SCHEDULE = [None for _ in range(N_STEPS)]  # To store the current schedule
BEST_SCHEDULE = [None for _ in range(N_STEPS)]  # To store the best feasible solution
PUMP_OPEN = LinkStatus.Open
PUMP_CLOSED = LinkStatus.Closed
ACTUATIONS_MAX = 3  # Max number of actuations for each pump


def get_constraints(wn: WaterNetworkModel):
    """
    Get the constraints for the network.
    """
    # TODO Check if these constraints are correct
    # TODO Check if they are already defined in the '.inp' file

    # Get tanks and nodes
    tanks = wn.tank_name_list
    tank_level_min_max = {tank: [wn.get_node(tank).min_level, wn.get_node(tank).max_level] for tank in tanks}

    cntr = {
        # Minimum pressure head at each node
        "pressure_min": {"90": 51, "50": 42, "170": 30},
        # Minimum and maximum tank level at each time step
        "tank_level_min_max": tank_level_min_max,
        # Initial tank level
        "tank_level_initial": {tank: wn.get_node(tank).init_level for tank in tanks},
    }

    return cntr


CNSTR = get_constraints(WN)


def pump_add_control(wn: WaterNetworkModel, pump_name: str, status: LinkStatus, time: int):
    # Retrieve the pump object
    pump = wn.get_link(pump_name)
    if pump is None:
        console.print(f"[red]Pump {pump_name} not found![/red]")
        return

    # Define the control action to change on simulation time
    threshold = max(0, time - 5)  # 5 seconds before the time step to be sure the control is applied
    condition = wntr.network.TimeOfDayCondition(model=wn, relation=Comparison.eq, threshold=threshold, repeat=False)

    # Define the control action
    action = ControlAction(target_obj=pump, attribute="status", value=status)

    # Create the control
    control = Control(condition=condition, then_action=action)

    # Add the control to the network
    wn.add_control(f"{time:05d}_{pump_name}_{status}", control)


# Print network info and parameters using rich table
def show_network_info(wn: wntr.network.WaterNetworkModel):
    """
    Print the network info and parameters.
    """
    console.print(f"Network name ..... [cyan]{wn.name}[/cyan]")
    console.print(f"Num pumps ........ [cyan]{N_PUMPS}[/cyan]")
    console.print(f"Num time steps ... [cyan]{N_STEPS}[/cyan]")
    console.print(f"Time step ........ [cyan]{TIME_STEP}[/cyan] seconds")


def get_energy_prices(verbose: bool = False) -> list[float]:
    """
    Get the energy prices of the network.
    """
    if "PRICES" not in WN.pattern_name_list:
        console.print("[red]Error:[/red] 'PRICES' pattern not found in the network.")
        exit(1)

    energy_prices = WN.get_pattern("PRICES").multipliers
    if verbose:
        for i in range(N_STEPS):
            console.print(f"Time step {i} ..... [cyan]{energy_prices[i]}[/cyan]")
    return energy_prices


# Print network info
show_network_info(WN)

# Get energy prices
ENERGY_PRICES = get_energy_prices(verbose=False)


def show_controls(wn: WaterNetworkModel):
    """
    Displays all controls in the WaterNetworkModel using the rich library.

    Parameters:
    -----------
    wn : WaterNetworkModel
        The water network model containing the controls to display.
    """
    console = Console()

    if not wn.controls:
        console.print(
            "[bold yellow]Oh no! No controls found in the network. Time to get those pumps partying![/bold yellow]"
        )
        return

    # Create a rich Table
    table = Table(title="Water Network Controls Overview", box=box.ROUNDED, expand=False)

    # Define table columns
    table.add_column("Control Name", style="cyan", no_wrap=True)
    table.add_column("Condition", style="magenta")
    table.add_column("Actions", style="green")
    table.add_column("Priority", style="yellow", justify="center")

    # Iterate over all controls and add rows to the table
    controls: List[Tuple[str, Control]] = list(wn.controls())
    controls = sorted(controls, key=lambda x: x[0])
    for control_name, control in controls:

        # Get condition description
        condition = str(control.condition)
        if len(condition) > 40:
            condition = condition[:37] + "..."

        # Get actions description
        actions = ", ".join([str(action) for action in control.actions()])
        if len(actions) > 50:
            actions = actions[:47] + "..."

        # Get priority
        priority = f"{control.priority.name}"

        # Add a row to the table
        table.add_row(control_name, condition, actions, priority)

    console.print(table)


def show_node_values(
    end_time: int,
    constraints: Dict[str, Dict[str, any]],
    initial_pressures: Dict[str, float],
    initial_tank_levels: Dict[str, float],
    pressures: Dict[str, float],
    tank_levels: Dict[str, float],
    is_final: bool,
):
    """
    Displays all values checked in the process_node function using the rich library,
    including start and end values for pressures and tank levels.

    Parameters:
    -----------
    end_time : int
        The current time step of the simulation.
    pressures : Dict[str, float]
        Dictionary containing current pressure values at each node.
    tank_levels : Dict[str, float]
        Dictionary containing current tank level (head) values.
    constraints : Dict[str, Dict[str, any]]
        Dictionary containing various constraints like pressure_min, tank_level_min_max, etc.
    initial_pressures : Dict[str, float]
        Dictionary containing initial pressure values at each node.
    initial_tank_levels : Dict[str, float]
        Dictionary containing initial tank levels.
    is_final : bool
        Flag indicating if this is the final simulation step.
    """

    # Separator
    console.rule(f"[bold yellow]Process_Node : End Time {end_time}[/bold yellow]")

    # Create and populate the Combined Table with Context
    combined_table = Table(
        title="💧 Water Network Simulation Results", box=box.ROUNDED, header_style="bold blue", expand=True
    )

    # Define table columns
    combined_table.add_column("Type", style="cyan", no_wrap=True, width=10)
    combined_table.add_column("Entity Name", style="cyan", no_wrap=True)
    combined_table.add_column("Attribute", style="magenta")
    combined_table.add_column("Initial Value (m)", style="cyan")
    combined_table.add_column("Current Value (m)", style="magenta")
    combined_table.add_column("Min Constraint (m)", style="green")
    combined_table.add_column("Max Constraint (m)", style="green", justify="center")
    combined_table.add_column("Status", style="bold")

    # Populate the table with Node Pressures
    for node_name, p_min in constraints.get("pressure_min", {}).items():
        p_initial = initial_pressures.get(node_name, "N/A")
        p_current = pressures.get(node_name, "N/A")

        # Determine Status
        if isinstance(p_current, float):
            if p_current < p_min:
                status = Text("🚨 Below Min", style="bold red")
            else:
                status = Text("✅ OK", style="bold green")
        else:
            status = Text("❓ Missing Data", style="bold yellow")

        combined_table.add_row(
            "Node",
            node_name,
            "Pressure",
            f"{p_initial:.2f}" if isinstance(p_initial, float) else "N/A",
            f"{p_current:.2f}" if isinstance(p_current, float) else "N/A",
            f"{p_min:.2f}",
            "-",  # No max constraint for pressure
            status,
        )

    # Populate the table with Tank Levels
    for tank_name, level_range in constraints.get("tank_level_min_max", {}).items():
        r_min, r_max = level_range
        r_initial = initial_tank_levels.get(tank_name, "N/A")
        r_current = tank_levels.get(tank_name, "N/A")

        # Determine Status
        if isinstance(r_current, float):
            if r_current < r_min:
                status = Text("🚨 Below Min", style="bold red")
            elif r_current > r_max:
                status = Text("🚨 Above Max", style="bold red")
            else:
                status = Text("✅ OK", style="bold green")
        else:
            status = Text("❓ Missing Data", style="bold yellow")

        combined_table.add_row(
            "Tank",
            tank_name,
            "Level",
            f"{r_initial:.2f}" if isinstance(r_initial, float) else "N/A",
            f"{r_current:.2f}" if isinstance(r_current, float) else "N/A",
            f"{r_min:.2f}",
            f"{r_max:.2f}",
            status,
        )

    # If it's the final step, check reservoir stability and add to the table
    if is_final and "stability" in constraints:
        for reservoir_name, r_min in constraints.get("stability", {}).items():
            r_initial = initial_tank_levels.get(reservoir_name, "N/A")
            r_current = tank_levels.get(reservoir_name, "N/A")

            # Determine Status
            if isinstance(r_current, float):
                if r_current < r_min:
                    status = Text("🚨 Below Min", style="bold red")
                else:
                    status = Text("✅ Stable", style="bold green")
            else:
                status = Text("❓ Missing Data", style="bold yellow")

            combined_table.add_row(
                "Reservoir",
                reservoir_name,
                "Level",
                f"{r_initial:.2f}" if isinstance(r_initial, float) else "N/A",
                f"{r_current:.2f}" if isinstance(r_current, float) else "N/A",
                f"{r_min:.2f}",
                "-",  # No max constraint for stability
                status,
            )

    # Render the combined table
    console.print(combined_table)


def process_node(node: dict, is_final: bool, verbose: bool = False) -> bool:
    """
    Process a node to check if it satisfies the constraints.
    """

    # Run simulation
    node["model"].options.time.duration = node["end_time"]
    sim = wntr.sim.EpanetSimulator(node["model"])
    out = sim.run_sim()

    # Get pressures
    pressures = out.node["pressure"].iloc[-1].to_dict()

    # Get tank levels
    tank_levels = out.node["head"].iloc[-1].to_dict()

    # Get initial pressures
    if node["step"] == 0:
        prev_pressures = out.node["pressure"].iloc[0].to_dict()
    else:
        prev_pressures = out.node["pressure"].iloc[-2].to_dict()

    # Get initial tank levels
    if node["step"] == 0:
        prev_tank_levels = out.node["head"].iloc[0].to_dict()
    else:
        prev_tank_levels = out.node["head"].iloc[-2].to_dict()

    if verbose:
        show_node_values(
            node["end_time"],
            CNSTR,
            prev_pressures,
            prev_tank_levels,
            pressures,
            tank_levels,
            is_final,
        )

    # Check pressure head constraints feasibility
    for node_name, p_min in CNSTR["pressure_min"].items():
        # Get the last pressure value for the node
        p_last = pressures.get(node_name, None)
        if p_last is None:
            console.print(f"[red]Pressure data for node {node_name} not found![/red]")
            return False
        # Infeasible due to pressure constraints
        if p_last < p_min:
            return False

    # Check tank level constraints feasibility
    for tank_name, r_min_max in CNSTR["tank_level_min_max"].items():
        r_last = tank_levels.get(tank_name, None)
        if r_last is None:
            console.print(f"[red]Tank level data for tank {tank_name} not found![/red]")
            return False
        r_min = r_min_max[0]
        r_max = r_min_max[1]
        # Infeasible due to tank level constraints
        if r_last < r_min or r_last > r_max:
            return False

    # Check reservoir stability constraints
    if is_final:
        for tank_name, r_min in CNSTR["stability"].items():
            r_last = tank_levels.get(tank_name, None)
            if r_last is None:
                console.print(f"[red]Tank level data for tank {tank_name} not found![/red]")
                return False
            # Infeasible due to tank level constraints
            if r_last < r_min:
                return False

    return True


def parse_actuations(y: int, actuations: list[int], x: list[int]) -> bool:
    """
    Parse the actuations from the y (number of open pumps) and x (pump statuses) values.
    """
    pumps_demand = y - np.sum(x)

    if pumps_demand == 0:
        return True  # No change needed

    pump_actuations = []  # list of tuples (pump_idx, actuations)

    if pumps_demand > 0:  # Need to open more pumps
        for i in range(N_PUMPS):
            if x[i] == 0 and actuations[i] < ACTUATIONS_MAX:
                pump_actuations.append((i, actuations[i]))

        if len(pump_actuations) < pumps_demand:
            return False  # Not enough pumps can be actuated

        # Sort by least actuations to distribute actuations evenly
        pump_actuations.sort(key=lambda x: x[1])

        for i in range(pumps_demand):
            pump_idx = pump_actuations[i][0]
            actuations[pump_idx] += 1
            x[pump_idx] = 1

    else:  # Need to close some pumps
        pumps_excess = -pumps_demand

        for i in range(N_PUMPS):
            if x[i] == 1 and actuations[i] > 0:
                pump_actuations.append((i, actuations[i]))

        if len(pump_actuations) < pumps_excess:
            return False  # Not enough pumps can be actuated

        # Sort by least actuations to distribute actuations evenly
        pump_actuations.sort(key=lambda x: x[1])

        for i in range(pumps_excess):
            pump_idx = pump_actuations[i][0]
            actuations[pump_idx] -= 1
            x[pump_idx] = 0

    return x


def get_score(lower_bound: float, depth: int) -> float:
    """
    Get the score of the node.
    """
    return OMEGA * lower_bound + (1 - OMEGA) * (-depth)


def create_child_node(parent_node: dict, y: int, verbose: bool = False) -> dict | None:
    """
    Create a child node from the parent node.
    """
    child_node = deepcopy(parent_node)
    child_node["step"] += 1
    child_node["depth"] += 1
    child_node["y"] = y
    child_node["end_time"] += TIME_STEP

    # Updates child_node["x"] and child_node["actuations"]
    is_valid = parse_actuations(y, child_node["actuations"], child_node["x"])
    if not is_valid:
        return None  # Invalid child node due to actuator constraints

    # Update lower bound (will be updated in process_node)
    child_node["lower_bound"] += y * ENERGY_PRICES[child_node["step"]]
    if child_node["lower_bound"] >= LOWER_BOUND:
        return None  # Infeasible by lower bound

    # Update score
    child_node["score"] = get_score(child_node["lower_bound"], child_node["depth"])

    # Update pump statuses in the network model
    wn = child_node["model"]
    x = child_node["x"]
    for pump_idx, pump in enumerate(PUMPS):
        status = PUMP_OPEN if x[pump_idx] else PUMP_CLOSED
        pump_add_control(wn, pump, status, parent_node["end_time"])

    if verbose:
        show_controls(wn)

    return child_node


def dfs(node: dict, verbose: bool = False):
    """
    Depth-first search to find the optimal solution.
    """
    global LOWER_BOUND
    global SCHEDULE
    global BEST_SCHEDULE

    # Prune the branch if the current lower bound exceeds the best found so far
    if node["lower_bound"] >= LOWER_BOUND:
        return

    # Check if node is a leaf node
    is_final = node["step"] == N_STEPS - 1

    # Process node
    is_feasible = process_node(node, is_final, verbose)
    console.print(f"is_feasible: {is_feasible}")
    if not is_feasible:
        return

    # Update schedule
    SCHEDULE[node["step"]] = node["x"]

    if is_final:
        # Update the best solution if this node has a lower cost
        if node["lower_bound"] < LOWER_BOUND:
            LOWER_BOUND = node["lower_bound"]
            BEST_SCHEDULE = deepcopy(SCHEDULE)
            console.print(f"[green]New best solution found![/green]")
            console.print(f"[green]   Cost: {LOWER_BOUND}[/green]")
        return

    # Branching: Iterate over possible number of pumps to open (0 to N_PUMPS)
    for y in range(N_PUMPS + 1):
        child_node = create_child_node(node, y, verbose)
        if child_node is None:
            continue  # Invalid child node
        dfs(child_node, verbose)  # Recursively explore the child node


def main():
    global LOWER_BOUND  # to update the lower bound in process_node
    global BEST_SCHEDULE
    verbose = True

    # Close all pumps initially
    for pump in WN.pump_name_list:
        WN.get_link(pump).initial_status = wntr.network.LinkStatus.Closed

    root_node = {
        "step": -1,
        "y": 0,  # y = number of OPEN pumps
        "x": [0] * N_PUMPS,  # x[i] = pump status list at time step i
        "actuations": [0] * N_PUMPS,  # number of times pump i has been actuated
        "lower_bound": 0,  # Initial lower bound
        "depth": 0,  # depth = depth of the current schedule
        "model": deepcopy(WN),  # copy of the water network model
        "end_time": 0,  # end time of the simulation
    }

    # Create child nodes
    for y in range(N_PUMPS + 1):
        child_node = create_child_node(root_node, y, verbose)
        if child_node is None:
            continue
        dfs(child_node, verbose)

    # Print the best solution
    if BEST_SCHEDULE:
        console.print(f"[bold green]Best solution found![/bold green]")
        console.print(f"[bold green]Cost: {LOWER_BOUND}[/bold green]")
        console.print(f"[bold green]Pump Schedule (Time Step : Pump Status):[/bold green]")
        for step, pump_status in enumerate(BEST_SCHEDULE, start=1):
            status_str = ", ".join([f"P{idx+1}: {'ON' if status else 'OFF'}" for idx, status in enumerate(pump_status)])
            console.print(f"Time {step}: {status_str}")
    else:
        console.print("[red]No feasible solution found.[/red]")


if __name__ == "__main__":
    main()
