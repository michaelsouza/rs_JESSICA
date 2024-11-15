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


def check_pressures(out, h: int):
    pass


def check_levels(out, h: int):
    pass


def check_stability(out, h: int):
    pass


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
        y, h = counter.y, counter.h
        is_feasible = update_x(x, y, h, max_actuations, verbose=False)

        # Display iteration details using Rich
        print(f"[bold blue]Iteration: {niter}[/bold blue]")
        print(f"y:{y[1:h+1]}, h: {h}, is_feasible: {is_feasible}")
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
        print(f"cost={cost}, cost_min={cost_min}")

        is_feasible = cost < cost_min
        if not is_feasible:
            # Use a warning icon with styled text for pruning due to cost
            print("[bold yellow]⚠️  Pruned: cost[/bold yellow]")
            continue

        if h == hmax and cost < cost_min:
            cost_min = cost
            # Use a checkmark icon with styled text for updating cost_min
            print(f"[bold green]✅ cost_min updated: {cost_min}[/bold green]")


if __name__ == "__main__":
    bbsolver()
