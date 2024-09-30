## B&B EPANET Pump Scheduling Optimization

The main focus of this project is to optimize the operation of pumps in a water distribution network, aiming to reduce energy consumption while ensuring the system operates safely and efficiently. This includes minimizing energy costs by considering dynamic energy tariffs, limiting the number of pump activations to reduce maintenance costs, and ensuring that operational constraints such as node pressures and tank levels are met.

The optimization problem is modeled with a branch-and-bound algorithm, and the hydraulic simulation is conducted using the EPANET solver.


## Equations for Pump Scheduling Optimization in Water Distribution Networks

### 1. Objective Function: Minimization of Energy Costs

The goal is to minimize the total energy cost over the scheduling horizon:

\[
\text{Minimize } z = \sum_{n=1}^{N} \sum_{t=1}^{T} C_{nt} E_{nt} X_{nt}
\]

Where:
- \( C_{nt} \) = energy tariff cost for pump \( n \) at time period \( t \).
- \( E_{nt} \) = energy consumed by pump \( n \) at time period \( t \).
- \( X_{nt} \in \{ 0, 1 \} \) = binary variable indicating whether pump \( n \) is ON (1) or OFF (0) at time period \( t \).

### 2. Operational and Hydraulic Constraints

#### 2.1 Reservoir Level Limits

The water level in each reservoir \( j \) must remain within allowable limits:

\[
S_{min,j} \leq S_{jt} \leq S_{max,j}, \forall j, \forall t
\]

Where \( S_{jt} \) is the water level of reservoir \( j \) at time period \( t \).

#### 2.2 Node Pressure Limits

The pressure at each critical node \( i \) must be within the permissible range:

\[
P_{min,i} \leq P_{it} \leq P_{max,i}, \forall i, \forall t
\]

Where \( P_{it} \) is the pressure at node \( i \) at time period \( t \).

#### 2.3 Accumulated Volume Constraint

The final water level in the reservoirs should be at least equal to the initial level to avoid depletion:

\[
S_{j,T} \geq S_{j,0}, \forall j
\]

#### 2.4 Pump Actuation Limits

The number of pump activations must be limited to minimize mechanical wear and maintenance costs:

\[
A_n = \sum_{t=1}^{T} (X_{nt} - X_{nt+1})^2, \forall n, \forall t
\]

Where \( A_n \) represents the total number of activations of pump \( n \).