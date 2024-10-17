# codes/bbsolver.py

import os
import numpy as np
from copy import deepcopy
from rich.console import Console
import wntr
import wntr.network

console = Console()

# Load network
FN = "networks/any-town.inp"
if not os.path.exists(FN):
    console.print(f"[red]Network file {FN} not found![/red]")
    exit(1)

# Define global parameters
WN = wntr.network.WaterNetworkModel(FN)
PUMPS = WN.pump_name_list
N_PUMPS = len(PUMPS)
TIME_STEP = 60 * 60  # 1 hour (in seconds)
N_STEPS = round(24 * 60 * 60 / TIME_STEP)  # number of time steps in a day
OMEGA = 0.5  # weight for the weighted best-first search
LOWER_BOUND = np.inf  # lower bound for the cost
SCHEDULE = [None for _ in range(N_STEPS)]  # To store the current schedule
BEST_SCHEDULE = [None for _ in range(N_STEPS)]  # To store the best feasible solution
PUMP_OPEN = wntr.network.LinkStatus.Open
PUMP_CLOSED = wntr.network.LinkStatus.Closed
ACTUATIONS_MAX = 3  # Max number of actuations for each pump


def get_constraints(wn: wntr.network.WaterNetworkModel):
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


# Print network info and parameters using rich table
def print_network_info(wn: wntr.network.WaterNetworkModel):
    """
    Print the network info and parameters.
    """
    console.print(f"Network name ..... [cyan]{wn.name}[/cyan]")
    console.print(f"# Pumps .......... [cyan]{N_PUMPS}[/cyan]")
    console.print(f"# Time steps ..... [cyan]{N_STEPS}[/cyan]")
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
print_network_info(WN)

# Get energy prices
ENERGY_PRICES = get_energy_prices(verbose=False)


def process_node(node: dict, is_final: bool) -> bool:
    """
    Process a node to check if it satisfies the constraints.
    """
    try:
        # Run simulation
        sim = wntr.sim.EpanetSimulator(node["model"])
        out = sim.run_sim()

        # Advance the simulation by one time step
        node["model"].options.time.duration += TIME_STEP

        # Get pressures
        pressures = out.node["pressure"].iloc[-1].to_dict()

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

        # Get tank levels
        tank_levels = out.node["head"].iloc[-1].to_dict()

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

    except Exception as e:
        console.print(f"[red]Error in processing node: {e}[/red]")
        return False


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


def create_child_node(parent_node: dict, y: int) -> dict | None:
    """
    Create a child node from the parent node.
    """
    child_node = deepcopy(parent_node)
    child_node["step"] += 1
    child_node["depth"] += 1
    child_node["y"] = y

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
    # TODO: Is this the correct way to update the pump statuses?
    # Should I use wntr.network.LinkStatus.Open or 1?
    # Can we change the initial_status if the sim is running?
    x = child_node["x"]
    for pump_idx, pump in enumerate(PUMPS):
        wn.get_link(pump).initial_status = PUMP_OPEN if x[pump_idx] else PUMP_CLOSED

    return child_node


def dfs(node: dict):
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
    is_feasible = process_node(node, is_final)
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
        child_node = create_child_node(node, y)
        if child_node is None:
            continue  # Invalid child node
        dfs(child_node)  # Recursively explore the child node


def main():
    global LOWER_BOUND  # to update the lower bound in process_node
    global BEST_SCHEDULE

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
        "current_time": 0,  # current time of the simulation
    }

    # Create child nodes
    for y in range(N_PUMPS + 1):
        child_node = create_child_node(root_node, y)
        if child_node is None:
            continue
        dfs(child_node)

    # Start DFS
    dfs(root_node)

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
