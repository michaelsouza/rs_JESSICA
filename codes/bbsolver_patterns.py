# Standard Library Imports
from copy import deepcopy

# Third-Party Imports
import numpy as np
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
import wntr
from wntr.network import LinkStatus, WaterNetworkModel
from wntr.network.controls import (
    ControlAction,
    Control,
    Comparison,
    TimeOfDayCondition,
)
from wntr.sim import EpanetSimulator
from rich import print

# Local Application/Library Imports
import economic_custom as ec
from economic_custom import pump_cost


def parse_y(y: list, max_actuations=3):
    num_activations = len(y)
    x = [
        [1, 0, 0],  # 1,1
        [1, 1, 0],  # 2,2
        [1, 0, 0],  # 1,3
        [1, 0, 1],  # 2,4
        [1, 0, 0],  # 1,5
        [1, 0, 0],  # 1,6
        [1, 0, 0],  # 1,7
        [1, 0, 0],  # 1,8
        [0, 0, 0],  # 0,9
        [0, 0, 0],  # 0,10
        [1, 1, 0],  # 2,11
        [1, 1, 0],  # 2,12
        [1, 1, 0],  # 2,13
        [1, 1, 0],  # 2,14
        [1, 1, 0],  # 2,15
        [1, 0, 0],  # 1,16
        [1, 0, 1],  # 2,17
        [1, 0, 0],  # 1,18
        [0, 0, 0],  # 0,19
        [0, 0, 0],  # 0,20
        [0, 0, 0],  # 0,21
        [1, 1, 0],  # 2,22
        [1, 0, 0],  # 1,23
        [0, 0, 0],  # 0,24
    ]

    for i in range(len(y)):
        assert y[i] == sum(x[i]), f"The schedule is incompatible"

    return x


def sim_run(
    wn: WaterNetworkModel,
    duration_hour: int,
    node_name_list: list,
    tank_name_list: list,
    fn_prefix: str,
    verbose: bool = False,
):
    wn.options.time.duration = duration_hour * 3600  # seconds
    sim = EpanetSimulator(wn)
    out = sim.run_sim()
    pressures = out.node["pressure"].to_dict()
    tank_levels = out.node["head"].to_dict()
    if not verbose:
        return pressures, tank_levels
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


def bbsolver():
    wn = WaterNetworkModel("networks/any-town.inp")
    node_name_list = ["55", "90", "170"]
    max_actuations = 1
    hmax = 3

    x = np.zeros((hmax + 1, 3), dtype=int)
    counter = BBCounter(hmax, max_actuations + 1)

    is_feasible = True
    niter = 0
    while counter.update_y(is_feasible):
        niter += 1
        print(f"iter: {niter}")
        y, h = counter.y, counter.h
        is_feasible = update_x(x, y, h, max_actuations)
        print(f"y:{y[1:h+1]}, h: {h}, is_feasible: {is_feasible}")
        show_x(x, h)

        if is_feasible:
            assert y[h] == sum(x[h]), f"y={y[h]} != sum(x)={sum(x[h])}"
        else:
            continue


if __name__ == "__main__":
    bbsolver()
