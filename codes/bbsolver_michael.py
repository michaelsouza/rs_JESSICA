# codes/bbsolver.py

import os
import heapq
import pandas as pd
import numpy as np
from copy import deepcopy
from rich.console import Console
import wntr

console = Console()

# load network
fn = 'networks/any-town.inp'
wn = wntr.network.WaterNetworkModel(fn)

# define global parameters
N_PUMPS = len(wn.pump_name_list)
T = 24 # number of time steps (1 hour each)
TIME_STEP = 3600 # 1 hour

# print network info and parameters using rich table
def print_network_info(wn):
    """
    Print the network info and parameters
    """
    console.print(f"Network name ..... [cyan]{wn.name}[/cyan]")
    console.print(f"# Pumps .......... [cyan]{N_PUMPS}[/cyan]")
    console.print(f"# Time steps ..... [cyan]{T}[/cyan]")
    console.print(f"Time step ........ [cyan]{TIME_STEP}[/cyan] seconds")

def get_energy_prices(wn, verbose=False):
    """
    Get the energy prices of the network.
    """
    price_pattern = wn.get_pattern('PRICES').multipliers
    energy_prices = []
    for i in range(T):
        index = i % len(price_pattern)
        energy_prices.append(price_pattern[index])
    if verbose: 
        for i in range(T):
            console.print(f"Time step {i} ..... [cyan]{energy_prices[i]}[/cyan]")
    return energy_prices

# print network info
print_network_info(wn)

# print energy prices
energy_prices = get_energy_prices(wn, verbose=False)

# Define the Optimization Problem
class PumpSchedulingProblem:
    def __init__(self, wn, energy_prices, N_pumps, T):
        self.wn = wn
        self.energy_prices = energy_prices
        self.N_pumps = N_pumps
        self.T = T
        self.max_actuations = 3  # As per the case study
        self.pump_names = wn.pump_name_list
        self.tank_names = wn.tank_name_list
        self.reservoir_names = wn.reservoir_name_list
        self.node_names = wn.junction_name_list + self.tank_names + self.reservoir_names
        self.pressure_constraints = self.get_pressure_constraints()
        self.tank_level_constraints = self.get_tank_level_constraints()
        self.initial_tank_levels = {tank: wn.get_node(tank).init_level for tank in self.tank_names}
    
    def get_pressure_constraints(self):
        # Define minimum pressure head requirements at critical nodes
        # For simplicity, assume a minimum pressure of 20 units at all nodes
        ''' PORQUE 20?  CONSULTAR EX ANY TOWN EPANET E AT(M) DO AUTOR R... AVALIAR VALORES MIN/MAX E CRÍTICOS CITADOS'''
        pressure_constraints = {node: 20 for node in self.node_names}
        # Specific nodes can have different constraints as needed
        pressure_constraints['90'] = 51
        pressure_constraints['50'] = 42
        pressure_constraints['170'] = 30
        return pressure_constraints
    
    def get_tank_level_constraints(self):
        tank_level_constraints = {}
        for tank_name in self.tank_names:
            tank = self.wn.get_node(tank_name)
            tank_level_constraints[tank_name] = {
                'min': tank.min_level,
                'max': tank.max_level
            }
        return tank_level_constraints


# Implement the Branch-and-Bound Algorithm with Incremental Bounding
class BranchAndBoundSolver:
    def __init__(self, problem):
        self.problem = problem
        self.best_solution = None
        self.best_cost = float('inf')
        self.priority_queue = []
        self.node_count = 0
    
    def initial_solution(self):
        # Generate an initial feasible solution using a greedy heuristic
        # For simplicity, run all pumps during off-peak hours
        y_initial = [0] * self.problem.T
        cost, feasible = self.evaluate_solution(y_initial)
        if feasible:
            self.best_solution = y_initial
            self.best_cost = cost
    
    def evaluate_solution(self, y_schedule):
        # Evaluate the total energy cost and check feasibility using EPANET simulator
        wn = deepcopy(self.problem.wn)
        sim = wntr.sim.EpanetSimulator(wn)
        
        # Set pump statuses according to y_schedule
        for t in range(self.problem.T):
            for i, pump_name in enumerate(self.problem.pump_names):
                pump = wn.get_link(pump_name)
                if i < y_schedule[t]:
                    pump_status = 'OPEN'
                else:
                    pump_status = 'CLOSED'
                pump.add_status(t * TIME_STEP, pump_status)
        
        # Run simulation
        try:
            results = sim.run_sim()
        except Exception as e:
            return float('inf'), False  # Infeasible solution
        
        # Check constraints
        pressures = results.node['pressure']
        tank_levels = results.node['pressure'].loc[:, self.problem.tank_names]
        
        # Check pressure constraints
        for node_name, min_pressure in self.problem.pressure_constraints.items():
            node_pressures = pressures.loc[:, node_name]
            if (node_pressures < min_pressure).any():
                return float('inf'), False  # Infeasible due to pressure constraints
        
        # Check tank level constraints
        for tank_name in self.problem.tank_names:
            tank = self.problem.wn.get_node(tank_name)
            min_level = tank.min_level
            max_level = tank.max_level
            tank_pressures = tank_levels.loc[:, tank_name]
            if ((tank_pressures < min_level).any() or (tank_pressures > max_level).any()):
                return float('inf'), False  # Infeasible due to tank level constraints
        
        # Compute energy cost
        power = results.link['power'].loc[:, self.problem.pump_names]
        total_energy = power.sum().sum() * (TIME_STEP / 3600)  # Convert to kWh
        total_cost = sum([
            self.problem.energy_prices[t] * power.iloc[t].sum() * (TIME_STEP / 3600)
            for t in range(self.problem.T)
        ])
        
        return total_cost, True
    
    def solve(self):
        self.initial_solution()
        # Initialize the priority queue with the root node
        root_node = {
            'level': 0,
            'y_schedule': [],
            'lower_bound': 0,
            'cost_so_far': 0,
            'tank_levels': self.problem.initial_tank_levels,
            'actuations': [0] * self.problem.N_pumps
        }
        heapq.heappush(self.priority_queue, (root_node['lower_bound'], self.node_count, root_node))
        self.node_count += 1
        
        while self.priority_queue:
            _, _, node = heapq.heappop(self.priority_queue)
            if node['level'] == self.problem.T:
                # Leaf node
                total_cost, feasible = self.evaluate_solution(node['y_schedule'])
                if feasible and total_cost < self.best_cost:
                    self.best_cost = total_cost
                    self.best_solution = node['y_schedule']
                continue
            
            # Branching
            for y in range(self.problem.N_pumps + 1):
                child_node = deepcopy(node)
                child_node['level'] += 1
                child_node['y_schedule'] = node['y_schedule'] + [y]
                # Update actuations
                if child_node['level'] > 1:
                    prev_y = node['y_schedule'][-1]
                    if y != prev_y:
                        for i in range(self.problem.N_pumps):
                            if (i < y) != (i < prev_y):
                                child_node['actuations'][i] += 1
                # Prune if actuations exceed max allowed
                if any(a > self.problem.max_actuations for a in child_node['actuations']):
                    continue
                # Incremental bounding (partial cost estimation)
                partial_cost = node['cost_so_far'] + y * self.problem.energy_prices[node['level']] * (TIME_STEP / 3600)
                child_node['cost_so_far'] = partial_cost
                lower_bound = partial_cost  # Since costs are additive and non-negative
                if lower_bound >= self.best_cost:
                    continue  # Prune the node
                # Add to priority queue
                heapq.heappush(self.priority_queue, (lower_bound, self.node_count, child_node))
                self.node_count += 1

def run_bbsolver():
    # Run the Algorithm and Output the Optimal Pump Schedule
    problem = PumpSchedulingProblem(wn, energy_prices, N_PUMPS, T)
    solver = BranchAndBoundSolver(problem)
    solver.solve()

    # Output the optimal schedule
    if solver.best_solution is not None:
        print("Optimal Pump Schedule:")
        for t, y in enumerate(solver.best_solution):
            print(f"Time {t}: Run {y} pumps")
        print(f"Total Cost: ${solver.best_cost:.2f}")
    else:
        print("No feasible solution found.")

def sim_iterate(wn:wntr.network.WaterNetworkModel, end_time:float, time_step:float, pressure_results:list, tank_level_results:list):
    # Update simulation times
    wn.options.time.duration = end_time
    wn.options.time.hydraulic_timestep = time_step
    wn.options.time.pattern_timestep = time_step
    wn.options.time.report_timestep = time_step
    
    # Run simulation
    sim = wntr.sim.EpanetSimulator(wn)
    results = sim.run_sim()
    
    # Extract pressures
    pressures = results.node['pressure'].iloc[-1].to_dict()
    pressure_results.append(pressures)
    
    # Extract tank levels
    tank_levels = results.node['pressure'].loc[:, wn.tank_name_list].iloc[-1].to_dict()
    tank_level_results.append(tank_levels)
    
    # Update initial conditions for the next simulation
    for tank in wn.tank_name_list:
        wn.get_node(tank).init_level = tank_levels[tank]

def process_node(wn:wntr.network.WaterNetworkModel, cnstr:dict, is_final:bool=False) -> bool:
    """
    Process a node to check if it satisfies the constraints.
    """
    # run simulation
    sim = wntr.sim.EpanetSimulator(wn)
    out = sim.run_sim()

    # get pressures    
    pressures = out.node['pressure'].iloc[-1].to_dict()
    
    # check pressure head constraints feasibility
    for node_name, p_min, p_max in cnstr['pressure'].items():
        # get the last pressure value for the node  
        p_last = pressures[node_name]
        if (p_last < p_min or p_last > p_max):
            return False  # Infeasible due to pressure constraints
    
    # get reservoir levels
    reservoir_levels = out.node['pressure'].loc[:, wn.reservoir_name_list].iloc[-1].to_dict()
    
    # check reservoir level constraints feasibility
    for reservoir_name, r_min, r_max in cnstr['reservoir'].items():
        # get the last reservoir level for the reservoir
        r_last = reservoir_levels[reservoir_name]
        if (r_last < r_min or r_last > r_max):
            return False  # Infeasible due to reservoir level constraints
    
    # check reservoir stability constraints
    if is_final:
        for reservoir_name, r_min in cnstr['stability'].items():
            # get the last reservoir level for the reservoir
            r_last = reservoir_levels[reservoir_name]
            if (r_last < r_min):
                return False  # Infeasible due to reservoir level constraints

    return True

def main_old():
    # Define simulation parameters
    total_duration = 24 * 3600 # 1 day
    time_step = 3600 # 1 hour
    num_steps = total_duration // time_step
    
    # Initialize results dictionaries
    tree_results = []

    # Main loop
    for step in range(1, int(num_steps) + 1):
        current_time = step * time_step
        print(f"Simulating time step {current_time / 3600} hours ...")

        # Update simulation times
        wn.options.time.duration = current_time
        wn.options.time.hydraulic_timestep = time_step
        wn.options.time.pattern_timestep = time_step
        wn.options.time.report_timestep = time_step
        
        # Run simulation
        sim = wntr.sim.EpanetSimulator(wn)
        results = sim.run_sim()
        
        # Extract pressures
        pressures = results.node['pressure'].iloc[-1].to_dict()
        pressure_results[step] = pressures
        
        # Extract tank levels
        tank_levels = results.node['pressure'].loc[:, wn.tank_name_list].iloc[-1].to_dict()
        tank_level_results[step] = tank_levels
        
        # Update initial conditions for the next simulation
        for tank in wn.tank_name_list:
            wn.get_node(tank).init_level = tank_levels[tank]
    
    # Convert results to pandas DataFrames
    pressure_df = pd.DataFrame(pressure_results).T
    tank_level_df = pd.DataFrame(tank_level_results).T
    
    # Save results to CSV files
    if not os.path.exists('results'):
        os.makedirs('results')

    pressure_df.to_csv('results/pressures.csv', index=False)
    tank_level_df.to_csv('results/tank_levels.csv', index=False)
    
    print("Simulation complete. Results saved to CSV files.")
        

def parse_actuations(cnstr:dict, y:int, actuations:list[int], x:list[int]) -> bool:
    """
    Parse the actuations from the y and x values.
    """
    sum_x = np.sum(x)
    pumps_demand = y - sum_x
    # if pumps_demand == 0, there is no need to update actuations
    if pumps_demand == 0:
        return False

    max_actuations:list[int] = cnstr['actuations']

    # (pump_idx, pump_actuations)
    pump_actuations = []

    if pumps_demand > 0: # we need to open more pumps        
        for i in range(N_PUMPS):
            # get the pumps that can be opened
            if x[i] == 0 and actuations[i] < max_actuations[i]:
                pump_actuations.append((i, actuations[i]))
            
        # if there are not enough pumps to be opened, return False
        if len(pump_actuations) < pumps_demand:
            return False
        
        # sort the pump actuations by the number of actuations
        # we are going to open the pumps with less actuations first
        pump_actuations.sort(key=lambda x: x[1])
        
        # update the actuations and x values
        for i in range(len(pump_actuations)):
            actuations[pump_actuations[i][0]] += 1
            x[pump_actuations[i][0]] = 1

    else: # if pumps_demand < 0, we need to close some pumps        
        # just to make the code cleaner
        pumps_excess = -pumps_demand

        # get the pumps that can be closed
        for i in range(len(x)):
            if x[i] == 1 and actuations[i] > 0:
                pump_actuations.append((i, actuations[i]))
        
        # sort pump_actuations by the number of actuations
        # we are going to close the pumps with less actuations first
        pump_actuations.sort(key=lambda x: x[1])
        
        # update the actuations and x values
        for i in range(pumps_excess):
            actuations[pump_actuations[i][0]] -= 1
            x[pump_actuations[i][0]] = 0
        
    return True

def main():
    # Define simulation parameters
    total_duration = 24 * 3600 # 1 day
    time_step = 3600 # 1 hour
    num_steps = total_duration // time_step
    
    # Initialize results dictionaries
    tree_results = []

    # STEP 1: SET ROOT NODE
    root_node = {
        'level': 0,
        # x(y) = [x1, x2, ..., xN], where 
        #    xi = 1 if pump i is open, and 0 otherwise
        'x': [0] * N_PUMPS, 
        # y = number of OPEN pumps
        'y': 0, 
        'actuations': [0] * N_PUMPS,
        'lower_bound': np.inf,
        'cost_so_far': 0,        
    }

    # STEP 2: CLOSE ALL PUMPS
    for pump in wn.pump_name_list:
        wn.get_link(pump).status = 'CLOSED'

    # Main loop
    for step in range(1, int(num_steps) + 1):
        current_time = step * time_step
        print(f"Simulating time step {current_time / 3600} hours ...")

        # Update simulation times
        wn.options.time.duration = current_time
        wn.options.time.hydraulic_timestep = time_step
        wn.options.time.pattern_timestep = time_step
        wn.options.time.report_timestep = time_step
        
        # Run simulation
        sim = wntr.sim.EpanetSimulator(wn)
        results = sim.run_sim()
        
        # Extract pressures
        pressures = results.node['pressure'].iloc[-1].to_dict()
        pressure_results[step] = pressures
        
        # Extract tank levels
        tank_levels = results.node['pressure'].loc[:, wn.tank_name_list].iloc[-1].to_dict()
        tank_level_results[step] = tank_levels
        
        # Update initial conditions for the next simulation
        for tank in wn.tank_name_list:
            wn.get_node(tank).init_level = tank_levels[tank]
    
    # Convert results to pandas DataFrames
    pressure_df = pd.DataFrame(pressure_results).T
    tank_level_df = pd.DataFrame(tank_level_results).T
    
    # Save results to CSV files
    if not os.path.exists('results'):
        os.makedirs('results')

    pressure_df.to_csv('results/pressures.csv', index=False)
    tank_level_df.to_csv('results/tank_levels.csv', index=False)
    
    print("Simulation complete. Results saved to CSV files.")

if __name__ == "__main__":
    main()