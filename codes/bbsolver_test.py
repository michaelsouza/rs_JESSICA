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


def model_add_pump_control(wn: WaterNetworkModel, pump_name: str, status: LinkStatus, time: float) -> str:
    # check if the control already exists
    control_name = f"{time:05d}_{pump_name}_{status}"
    controls = list(wn.controls())
    for name, control in controls:
        if name == control_name:
            print(f"Control {control_name} already exists")
            return
    pump = wn.get_link(pump_name)
    threshold = time
    condition = TimeOfDayCondition(wn, Comparison.eq, threshold)
    action = ControlAction(target_obj=pump, attribute="status", value=status)
    control = Control(condition, then_action=action)
    wn.add_control(control_name, control)
    return control_name


def model_clean_controls(wn: WaterNetworkModel):
    control_name_list = [control_name for control_name, _ in wn.controls()]
    for control_name in control_name_list:
        print(f"Removing control {control_name}")
        # wn.remove_control(control_name)


def sim_create_df(pressures, tank_levels, node_name_list, tank_name_list):
    pressures_df = {"node": [], "time": [], "pressure": []}
    for node, pressure in pressures.items():
        if node not in node_name_list:
            continue
        for t, p in pressure.items():
            pressures_df["node"].append(node)
            pressures_df["time"].append(t)
            pressures_df["pressure"].append(p)
    pressures_df = pd.DataFrame(pressures_df)

    tank_levels_df = {"tank": [], "time": [], "level": []}
    for tank, level in tank_levels.items():
        if tank not in tank_name_list:
            continue
        for t, l in level.items():
            tank_levels_df["tank"].append(tank)
            tank_levels_df["time"].append(t)
            tank_levels_df["level"].append(l)
    tank_levels_df = pd.DataFrame(tank_levels_df)
    return pressures_df, tank_levels_df


def sim_plot(pressures_df, tank_levels_df, duration, fn_prefix: str):
    # Create subplots
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 4), sharex=True)
    fig.suptitle(f"Pressures and Tank Levels (duration: {duration}s)")

    # Plot pressures
    sns.lineplot(data=pressures_df, x="time", y="pressure", hue="node", ax=ax1)
    ax1.set_title("Pressures")
    ax1.set_ylabel("Pressure")

    # Plot tank levels
    sns.lineplot(data=tank_levels_df, x="time", y="level", hue="tank", ax=ax2)
    ax2.set_title("Tank Levels")
    ax2.set_xlabel("Time")
    ax2.set_ylabel("Level")

    plt.tight_layout()
    
    # Save to file instead of showing
    plt.savefig(fn_prefix + ".png")
    plt.close()  # Close the figure to free memory


def sim_run(wn: WaterNetworkModel, duration_hour: int, node_name_list: list, tank_name_list: list, fn_prefix: str, verbose: bool = False):    
    wn.options.time.duration = duration_hour * 3600 # seconds
    sim = EpanetSimulator(wn)
    out = sim.run_sim()
    pressures = out.node["pressure"].to_dict()
    tank_levels = out.node["head"].to_dict()
    if not verbose:
        return pressures, tank_levels

    pressures_df, tank_levels_df = sim_create_df(pressures, tank_levels, node_name_list, tank_name_list)

    # convert time to hours
    pressures_df["time"] = pressures_df["time"] / 3600
    tank_levels_df["time"] = tank_levels_df["time"] / 3600

    # plot
    sim_plot(pressures_df, tank_levels_df, duration_hour, fn_prefix)
    
    # export to csv
    with open(fn_prefix + "_pressures.csv", "w") as f:
        pressures_df.to_csv(f, index=False)
    with open(fn_prefix + "_tank_levels.csv", "w") as f:
        tank_levels_df.to_csv(f, index=False)

    return out


def test_cost():
    wn = WaterNetworkModel("/home/michael/gitrepos/rs_JESSICA/networks/any-town.inp")
    node_name_list = ["55", "90", "170"]
    pump_name_list = sorted(wn.pump_name_list, key=lambda x: int(x))
    tank_name_list = sorted(wn.tank_name_list, key=lambda x: int(x))

    y = [1, 2, 1, 2, 1, 1, 1, 1, 0, 0, 2, 2, 2, 2, 2, 1, 2, 1, 0, 0, 0, 2, 1, 0]
    x = parse_y(y)

    control_name_list = []
    model_clean_controls(wn)
    # for i in range(len(x)):
    #     t = 3600 * i
    #     for j in range(len(x[i])):
    #         xij = x[i][j]
    #         linkStatus = LinkStatus.Opened if xij == 1 else LinkStatus.Closed
    #         control_name_list.append(model_add_pump_control(wn, pump_name_list[j], linkStatus, t))

    fn_prefix = "test_cost"     
    hour = 8
    result = sim_run(wn, hour, node_name_list, tank_name_list, fn_prefix + f"_{hour}", True)
    cost = pump_cost(result, wn)
    print(f"cost: {cost}")


if __name__ == "__main__":
    test_cost()
