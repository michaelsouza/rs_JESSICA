# economic_custom.py

from wntr.network import WaterNetworkModel
from wntr.sim.results import SimulationResults
import numpy as np
import pandas as pd
from pandas import DataFrame
import logging
import scipy

logger = logging.getLogger(__name__)

def pump_power(flowrate: DataFrame, head: DataFrame, wn: WaterNetworkModel) -> DataFrame:
    """
    Compute pump power.

    The computation uses pump flow rate, node head (used to compute headloss at
    each pump), and pump efficiency. Pump efficiency is defined in
    ``wn.options.energy.global_efficiency``. Pump efficiency curves are currently
    not supported.

        wn.options.energy.global_efficiency = 75 # This means 75% or 0.75

    Parameters
    ----------
    flowrate : pandas DataFrame
        A pandas DataFrame containing pump flowrates
        (index = times, columns = pump names).

    head : pandas DataFrame
        A pandas DataFrame containing node head
        (index = times, columns = node names).

    wn: wntr WaterNetworkModel
        Water network model.  The water network model is needed to
        define energy efficiency.

    Returns
    -------
    A DataFrame that contains pump power in W (index = times, columns = pump names).
    """

    pumps = wn.pump_name_list
    time = flowrate.index

    # Initialize a DataFrame to store headloss for each pump at each time step
    headloss = pd.DataFrame(data=None, index=time, columns=pumps)

    # Calculate headloss for each pump based on the flowrate and node head
    for pump_name, pump in wn.pumps():
        start_node = pump.start_node_name
        end_node = pump.end_node_name
        start_head = head.loc[:, start_node]
        end_head = head.loc[:, end_node]
        node_headloss = end_head - start_head
        headloss.loc[:, pump_name] = node_headloss

    # Initialize a dictionary to store efficiency for each pump at each time step
    efficiency_dict = {}
    for pump_name, pump in wn.pumps():
        if pump.efficiency is None:
            # If no efficiency curve is defined, use the global efficiency
            efficiency_dict[pump_name] = [wn.options.energy.global_efficiency / 100.0 for i in time]
        else:
            # If an efficiency curve is defined, interpolate the efficiency based on flowrate
            curve = wn.get_curve(pump.efficiency)
            x = [point[0] for point in curve.points]
            y = [point[1] / 100.0 for point in curve.points]
            interp = scipy.interpolate.interp1d(x, y, kind="linear", fill_value="extrapolate")
            efficiency_dict[pump_name] = interp(np.array(flowrate.loc[:, pump_name]))

    # Convert the dictionary to a DataFrame
    efficiency = pd.DataFrame(data=efficiency_dict, index=time, columns=pumps)

    # Replace zero efficiency with 1 to avoid division by zero
    efficiency.replace(0, 1, inplace=True)

    # Calculate power using the formula: Power = (Flowrate * Headloss * Gravity * Density) / Efficiency
    # Constants: Gravity = 9.81 m/s², Density of water = 1000 kg/m³
    pumps_flowrate = flowrate.loc[:, pumps]
    pumps_headloss = headloss.loc[:, pumps]
    power = 1000.0 * 9.81 * pumps_headloss * pumps_flowrate / efficiency  # Watts = J/s

    return power


def pump_energy(flowrate: DataFrame, head: DataFrame, wn: WaterNetworkModel) -> DataFrame:
    """
    Compute the pump energy over time.

    The computation uses pump flow rate, node head (used to compute headloss at
    each pump), and pump efficiency. Pump efficiency is defined in
    ``wn.options.energy.global_efficiency``. Pump efficiency curves are currently
    not supported.

        wn.options.energy.global_efficiency = 75 # This means 75% or 0.75

    Parameters
    ----------
    flowrate : pandas DataFrame
        A pandas DataFrame containing pump flowrates
        (index = times, columns = pump names).

    head : pandas DataFrame
        A pandas DataFrame containing node head
        (index = times, columns = node names).

    wn: wntr WaterNetworkModel
        Water network model.  The water network model is needed to
        define energy efficiency.

    Returns
    -------
    A DataFrame that contains pump energy in J (index = times, columns = pump names).
    """

    # Calculate instantaneous power consumption
    power = pump_power(flowrate, head, wn)  # Watts = J/s

    # Calculate energy by multiplying power with the timestep duration
    energy = power * wn.options.time.report_timestep  # J = Ws

    return energy


def pump_cost(result: SimulationResults, wn: WaterNetworkModel, energy: DataFrame = None) -> float:
    """
    Compute the pump cost over time.

    Energy cost is defined in ``wn.options.energy.global_price``. Pump energy
    price and price patterns are currently not supported.

        wn.options.energy.global_price = 3.61e-8  # $/J; equal to $0.13/kW-h

    Parameters
    ----------
    energy : pandas DataFrame
        A pandas DataFrame containing pump energy (J), computed from ``wntr.metrics.pump_energy``
        (index = times, columns = pump names).

    wn: wntr WaterNetworkModel
        Water network model.  The water network model is needed to
        define pump enery prices and patterns.

    Returns
    -----------
    float: total pump cost
        Total pump operational cost in dollars ($)
    """
    if energy is None:
        # Calculate pump energy if not provided
        energy = pump_energy(result.link["flowrate"], result.node["head"], wn)

    time = energy.index
    # The last timestep is not used for cost calculation, because
    # it is used to set the time duration
    num_timesteps = len(time) - 1

    # Retrieve the energy price pattern, if any
    # The price pattern "PRICES" should be defined in the input file
    # Prices are converted from cents per kWh to dollars per Joule
    try:
        price_pattern = wn.get_pattern("PRICES")
        multipliers = (price_pattern.multipliers[:num_timesteps] / 100.0) / (1000 * 3600)
    except KeyError:
        raise KeyError("Price pattern 'PRICES' not found in the network.")

    # Get the energy matrix
    energy_matrix = energy.to_numpy().T  # shape (num_pumps, num_timesteps)

    # Consider only the first num_timesteps rows
    energy_matrix = energy_matrix[:, :num_timesteps]

    # Sum up the energy consumption for all pumps and multiply by the price per Joule
    cost = (energy_matrix * multipliers).sum()

    return cost
