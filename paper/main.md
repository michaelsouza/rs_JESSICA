# A Branch-and-Bound Algorithm for Optimal Pump Scheduling in Water Distribution Networks

## Abstract


## Introduction
Water System Network (WSN) is a complex network of pipes, pumps, and valves that deliver water from the source to the consumers. The WSN is designed to meet the demand of the consumers while minimizing the cost of the water supply. The WSN is a complex network of pipes, pumps, and valves that deliver water from the source to the consumers.

## Methods

### Problem Formulation

#### Notations
- $\mathcal{R}$ is the set of reservoirs.
- $\mathcal{P}$ is the set of pumps.
- $N=|\mathcal{P}|$ is the number of pumps 
- $T$ is the number of time steps.
- $x_{it} \in \{0,1\}$ is a binary variable that indicates if pump $i$ is on ($x_{it} = 1$) or off ($x_{it} = 0$) at time step $t$.
- $e_{it}$ is the energy (kWh) consumption of pump $i$ at time step $t$.
- $c_{it}$ is the energy tariff ($) of pump $i$ at time step $t$.
- $p^{\min}_{i}$ and $p^{\max}_{i}$ are the minimum and maximum power requirements at node $i$, respectively.
- $l^{\min}_{j}$ and $l^{\max}_{j}$ are the minimum and maximum level of the reservoir at node $j$, respectively.
- $a_{i}$ is the number of actuation of pump $i$.
- $a^{\max}_{i}$ is the maximum number of actuation of pump $i$.

The number of actuation of pump $i$ is given by:
$$
a_{i} = \sum_{t=1}^{T-1} \max(x_{i,t+1} - x_{it}, 0)
$$

#### Formulation
The objective is to minimize the total energy consumption of the pumps while satisfying the demand at all nodes and the pressure head constraints at all nodes. 

$$
\begin{alignat}{2}
& \underset{x}{\text{minimize}} && z = \sum_{i=1}^{N} \sum_{t=1}^{T} c_{it} e_{it} x_{it} \\
& \text{subject to}\quad  &&p^{\min}_{i} \leq p_{it} \leq p^{\max}_{i}, \; \forall i \in P, \forall t \in T \\
&&& a_{i} \leq a^{\max}_{i}, \; \forall i \in P \\
&&& l^{\min}_{j} \leq l_{jt} \leq l^{\max}_{j}, \; \forall j \in \mathcal{R} \\
&&& s_{j0} \leq s_{jT}, \; \forall j \in \mathcal{R} \\
&&& x_{it} \in \{0,1\}, \; \forall i \in P, \forall t \in T
\end{alignat}
$$


#### Constraints

##### - **Pressure Head Constraints**

$$
p^{\min}_{i} \leq p_{it} \leq p^{\max}_{i}, \quad \forall i \in \mathcal{P}, \forall t \in T
$$

To maintain the water pressure within acceptable limits at each pump node over all time steps. 

##### - **Pump Actuation Constraints**

$$
a_{i} \leq a^{\max}_{i}, \quad \forall i \in \mathcal{P}
$$

Limits the number of times each pump can be turned on and off (actuated) during the scheduling period.

##### - **Reservoir Level Constraints**

$$
l^{\min}_{j} \leq l_{jt} \leq l^{\max}_{j}, \quad \forall j \in \mathcal{R}, \forall t \in T
$$

Maintains reservoir levels within predefined bounds to ensure adequate water supply and system stability.


##### - **Reservoir Stability Constraint**

$$
s_{j0} \leq s_{jT}, \quad \forall j \in \mathcal{R}
$$

Ensures that the reservoir levels at the end of the scheduling period are at least as high as they were at the beginning. It ensures long-term viability and avoids scenarios where reservoirs are exhausted, requiring significant time or resources to replenish.

### Simplifications
We make the following assumptions to simplify the problem:

1. All pumps are identical.
2. There are 48 time steps in a day (30 minutes each).

By making these assumptions, we can 
1. replace the decision variable $x_{it}$ by $y_{t}$ representing the number of pumps that are on at time step $t$.
2. replace the decision variable $e_{it}$ by $e_{t}$ representing the total energy consumption at time step $t$.

With these simplifications, the problem can be formulated as 

$$
\begin{alignat}{2}
& \underset{y, e}{\text{minimize}} && z = \sum_{t=1}^{T} c_{t} e_{t} y_t\\
& \text{subject to}\quad  &&p^{\min}_{i} \leq p_{it} \leq p^{\max}_{i}, \; \forall i \in P, \forall t \in T \\
&&& a_{i} \leq a^{\max}_{i}, \; \forall i \in P \\
&&& l^{\min}_{j} \leq l_{jt} \leq l^{\max}_{j}, \; \forall j \in \mathcal{R} \\
&&& s_{j0} \leq s_{jT}, \; \forall j \in \mathcal{R} \\
&&& x_{it} \in \{0,1\}, \; \forall i \in P, \forall t \in T
\end{alignat}
$$



### Branch-and-Bound Algorithm

### Implementation

## Results


