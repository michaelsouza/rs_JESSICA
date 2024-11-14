import wntr
from wntr.network import LinkStatus, WaterNetworkModel
from wntr.network.controls import ControlAction, Control, Comparison
from wntr.sim import EpanetSimulator
from rich import print
import pandas as pd
from copy import deepcopy
import economic_custom as ec
from wntr.network.controls import ControlAction, Control, TimeOfDayCondition, Comparison
from economic_custom import pump_cost
import numpy as np

import seaborn as sns
import matplotlib.pyplot as plt


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


def update_x(x: np.ndarray, y: np.ndarray, h: int, max_actuations: int) -> bool:
    # number of actuations in hour h - 1
    if y[h] == y[h - 1]:
        x[h, :] = x[h - 1, :]
        return True
    # reset the actuations for hour h
    x[h, :] = 0
    
    # calculate the cumulative actuations
    actuations_csum = calc_actuations_csum(x, h)
    
    # sorted from least actuations to most
    pumps_to_actuate = np.argsort(actuations_csum)

    if y[h] > y[h - 1]:
        num_actuations = y[h] - y[h - 1]
        pumps_to_actuate = pumps_to_actuate[:num_actuations]
        if any(actuations_csum[pumps_to_actuate] == max_actuations):
            return False
        x[h, pumps_to_actuate] = 1
        return True

    if y[h] < y[h - 1]:
        num_actuations = y[h - 1] - y[h]
        pumps_to_deactuate = pumps_to_actuate[:num_actuations]
        x[h, pumps_to_deactuate] = 0
        return True
    return True


def bbsolver():
    wn = WaterNetworkModel("/home/michael/github/rs_JESSICA/networks/any-town.inp")
    node_name_list = ["55", "90", "170"]
    max_actuations = 1
    hmax = 3

    x = np.zeros((hmax + 1, 3), dtype=int)
    counter = BBCounter(hmax, max_actuations)

    is_feasible = True
    niter = 0
    while counter.update_y(is_feasible):
        niter += 1
        print(f"iter: {niter}")
        y, h = counter.y, counter.h
        is_feasible = update_x(x, y, h, max_actuations)
        print(f"y:{y[1:h+1]}, h: {h}, is_feasible: {is_feasible}")
        show_x(x, h)
        if not is_feasible:
            continue


if __name__ == "__main__":
    bbsolver()
