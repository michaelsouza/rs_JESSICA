# A Branch-and-Bound Algorithm for Optimal Pump Scheduling in Water Distribution Networks

## Abstract


## Introduction
Water System Network (WSN) is a complex network of pipes, pumps, and valves that deliver water from the source to the consumers. The WSN is designed to meet the demand of the consumers while minimizing the cost of the water supply.

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
- $p^{\min}_{i}$ and $p^{\max}_{i}$ are the minimum and maximum pressure head requirements at node $i$, respectively.
- $l^{\min}_{j}$ and $l^{\max}_{j}$ are the minimum and maximum level of the reservoir at node $j$, respectively.
- $a_{i}$ is the number of actuation of pump $i$.
- $a^{\max}_{i}$ is the maximum number of actuation of pump $i$.

The number of actuation of pump $i$ is given by:
$$
a_{i} = \sum_{t=1}^{T-1} \max(x_{i,t+1} - x_{it}, 0)
$$

#### Formulation
The objective is to minimize the total energy consumption of the pumps while satisfying the constraints over all time steps.

$$
\begin{alignat}{2}
& \underset{x}{\text{minimize}} && z = \sum_{i=1}^{N} \sum_{t=1}^{T} c_{it} e_{it} x_{it} \\
& \text{subject to}\quad  &&p^{\min}_{i} \leq p_{it} \leq p^{\max}_{i}, \; \forall i \in \mathcal{P}, \forall t \in \{1, \ldots, T\} \\
&&& a_{i} \leq a^{\max}_{i}, \; \forall i \in \mathcal{P} \\
&&& l^{\min}_{j} \leq l_{jt} \leq l^{\max}_{j}, \; \forall j \in \mathcal{R}, \forall t \in \{1, \ldots, T\} \\
&&& s_{j0} \leq s_{jT}, \; \forall j \in \mathcal{R} \\
&&& x_{it} \in \{0,1\}, \; \forall i \in \mathcal{P}, \forall t \in \{1, \ldots, T\}
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
l^{\min}_{j} \leq l_{jt} \leq l^{\max}_{j}, \quad \forall j \in \mathcal{R}, \forall t \in \{1, \ldots, T\}
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
2. replace the decision variable $e_{it}$ by $e_{t}$ representing the energy consumption at time step $t$.

With these simplifications, the problem can be formulated as 

$$
\begin{alignat}{2}
& \underset{y}{\text{minimize}} && z = \sum_{t=1}^{T} c_{t} e_{t} y_t\\
& \text{subject to}\quad  &&p^{\min}_{i} \leq p_{it} \leq p^{\max}_{i}, \; \forall i \in P, \forall t \in T \\
&&& a_{i} \leq a^{\max}_{i}, \; \forall i \in P \\
&&& l^{\min}_{j} \leq l_{jt} \leq l^{\max}_{j}, \; \forall j \in \mathcal{R} \\
&&& s_{j0} \leq s_{jT}, \; \forall j \in \mathcal{R} \\
&&& y_{t} \in \{0,1,\ldots, N\}, \; \forall t \in \{1, \ldots, T\}
\end{alignat}
$$

To derive $x_{it}$ from $y_t$, we minimize the total number of actuations by sequentially activating pumps. For example, if $y_t = 2$, we activate pumps 1 and 2 at time step $t$ and, if $y_{t+1} = 3$, we activate pumps 1, 2 and 3 at time step $t+1$. In this process, we try to minimize the number of actuations by turning on the first pump and then the second and so on, until the maximum number of actuations is reached.

### Branch-and-Bound Algorithm

To efficiently solve the optimal pump scheduling problem formulated above, we employ a **Branch-and-Bound (B&B)** algorithm. The B&B method is particularly suited for integer programming problems, such as our pump scheduling model, where decision variables are discrete (i.e., the number of pumps operating at each time step). The algorithm systematically explores the solution space by partitioning it into smaller subproblems (branching) and calculating bounds to eliminate suboptimal solutions (bounding), thereby reducing the computational effort required to find the optimal solution.

#### Overview of the Branch-and-Bound Process

The B&B algorithm operates by maintaining a tree structure where each node represents a subproblem with certain decision variables fixed. The process involves the following key steps:

1. **Initialization**: Start with the original problem as the root node.
2. **Branching**: Divide the current subproblem into smaller subproblems by fixing one or more decision variables.
3. **Bounding**: Calculate lower and upper bounds for each subproblem to estimate the best possible objective value within that subproblem.
4. **Pruning**: Discard subproblems that cannot yield better solutions than the best-known solution (based on bounds).
5. **Selection**: Choose the next subproblem to explore, typically using strategies like best-first or depth-first search.
6. **Termination**: The algorithm terminates when all subproblems have been either solved or pruned, ensuring that the optimal solution has been found.

#### Application to Pump Scheduling

The B&B algorithm manages the discrete decision variables $ y_t $, representing the number of pumps running at each time step $ t $. We use an incremental approach to solve the problem step-by-step, calculating partial costs as lower bounds. Energy consumption and feasibility constraints are evaluated using the EPANET simulator, requiring the solver to calculate costs and check feasibility at each step.

##### 1. Initialization

- **Root Node**: The root node represents the entire feasible solution space without any pumps scheduled. At this stage, no $ y_t $ variables are fixed.
- **Initial Bounds**:
  - **Lower Bound**: Initialize the lower bound to zero, as no energy has been consumed yet.
  - **Upper Bound**: Generate an initial feasible solution using a heuristic (e.g., greedy algorithm) where pumps are scheduled based on energy tariffs or demand patterns. The objective value of this solution serves as the initial upper bound on energy consumption.

##### 2. Branching Strategy

To efficiently navigate the solution space, we adopt a **variable selection strategy** that chooses the most promising $ y_t $ to branch on at each step. Since all $ y_t $ variables have the same number of constraints, we can use a straightforward approach:

- **Sequential Selection**: Select the time step $ t $ in a sequential manner, starting from the first time step and moving to the next.

Once a variable $ y_t $ is selected, branching is performed by creating child nodes that fix $ y_t $ to specific integer values within its feasible range $ \{0, 1, \ldots, N\} $. For example, if $ y_t $ can take values from 0 to 3, four child nodes are created with $ y_t = 0 $, $ y_t = 1 $, $ y_t = 2 $, and $ y_t = 3 $, respectively.

##### 3. Bounding

We use an **incremental bounding approach** to compute lower bounds. This involves solving the scheduling problem step-by-step, calculating partial costs at each timestep. The EPANET simulator evaluates energy consumption and feasibility at each step, ensuring accurate partial costs that reflect real-world performance.

###### Incremental Bounding Process

1. **Sequential Timestep Solving**: For a given subproblem, solve the pump scheduling up to the current timestep $ t $. This involves determining the optimal number of pumps $ y_1, y_2, \ldots, y_t $ that minimizes the cumulative energy cost up to time $ t $, considering all constraints up to that timestep.
   
2. **Partial Cost Accumulation**: The objective value obtained from solving up to timestep $ t $ represents the partial cost $ z_t $. This partial cost serves as the lower bound for the subproblem because any feasible solution extending beyond timestep $ t $ will incur additional costs.
   
3. **Total Lower Bound**: Sum the partial costs across all timesteps to obtain the total lower bound for the subproblem. Specifically, for a subproblem with fixed $ y_{t'} $ for $ t' \leq t $, the lower bound is:
   $$
   \text{Lower Bound} = \sum_{t'=1}^{t} z_{t'}
   $$
   
4. **Feasibility Check**: Ensure that the partial solution up to timestep $ t $ does not violate any constraints. If infeasibility is detected at any point, the subproblem is pruned.

###### Advantages of Incremental Bounding

Incremental bounding offers several advantages. First, it provides tighter bounds. By solving the problem incrementally, the lower bounds are more accurate compared to relaxed continuous solutions, leading to more effective pruning. Second, it ensures feasibility assurance. Each incremental step ensures that partial solutions are feasible, reducing the likelihood of exploring infeasible regions of the solution space.

##### 4. Pruning Criteria

A subproblem is pruned (i.e., discarded) if:

- **Bound Comparison**: The lower bound of the subproblem is greater than or equal to the current best upper bound. This indicates that no better solution can be found within this subproblem.
  
- **Infeasibility**: The subproblem violates any of the problem constraints, making it impossible to yield a feasible solution.

##### 5. Node Selection Strategy

To optimize the search process, we employ a hybrid node selection strategy that combines aspects of Best-First Search with consideration for the depth of nodes in the search tree:

- **Weighted Best-First Search (WBS)**: This strategy prioritizes nodes based on a weighted combination of their lower bounds and their depth in the search tree. The selection criterion for a node can be expressed as:

  $$ \text{Score} = w \cdot \text{LowerBound} + (1-w) \cdot (-\text{Depth}) $$

  where:
  - $w$ is a weighting factor between 0 and 1
  - LowerBound is the node's lower bound on the objective value
  - Depth is the node's depth in the search tree

  Nodes with lower scores are prioritized for exploration. This approach offers several advantages:
  
  1. It balances the exploration of promising solutions (low lower bounds) with the desire to reach complete solutions quickly (greater depth).
  2. By adjusting the weight $w$, we can fine-tune the trade-off between breadth and depth in the search.
  3. It helps prevent the algorithm from getting stuck exploring many shallow nodes with slightly better bounds.

- **Dynamic Weighting**: The weight $w$ can be adjusted dynamically during the search process. For example, it could start with a higher value to focus on promising areas of the solution space, and gradually decrease to encourage deeper exploration as the search progresses.

In this implementation, we utilize **Best-First Search** to prioritize promising subproblems and expedite convergence to the optimal solution.

##### 6. Solution Update and Termination

- **Solution Update**: Whenever a feasible integer solution is found, update the upper bound with its objective value. This tightens the bounds for subsequent pruning.
  
- **Termination**: The algorithm concludes when all nodes have been either explored or pruned. The best feasible solution encountered during the search is guaranteed to be optimal.

#### Algorithmic Steps for Pump Scheduling

Below is a detailed outline of the modified B&B algorithm tailored for the pump scheduling problem with incremental bounding:

1. **Initialize**:
   - Set the root node with no fixed $y_t$ (number of pumps running at each time step).
   - Compute an initial feasible solution using a greedy heuristic based on the WBS strategy.
   - Initialize the best solution as the heuristic solution.
   - Initialize a priority queue to manage nodes based on their weighted scores (combining lower bounds and tree depth).

2. **Loop**:
   - **Select Node**: Choose the node with the lowest weighted score from the priority queue.
   - **Check Termination**: If the node's lower bound is greater than or equal to the current upper bound, prune the node and continue.
   - **Branching**:
     - Select the next unfixed $y_t$ to branch on using the sequential selection strategy.
     - For each feasible value of $y_t$ (from 0 to $N$), create a child node with $y_t$ fixed to that value.
   - **Bounding for Each Child Node**:
     - **Incremental Solving**: Use EPANET simulator to solve the scheduling problem up to the current timestep $t$ with $y_t$ fixed.
     - **Compute Partial Cost**: Calculate the partial cost $z_t = c_t e_t y_t$ from the EPANET simulation results.
     - **Update Lower Bound**: Sum the partial costs to obtain the lower bound for the child node.
     - **Feasibility Check**: Ensure that the partial solution satisfies pressure head, pump actuation, and reservoir level constraints.
     - **Prune or Enqueue**:
       - If the lower bound is less than the current upper bound and the subproblem is feasible, calculate the node's weighted score and enqueue it.
       - Otherwise, prune the child node.
    **Bounding for Each Child Node**:

   - **Update Best Solution**: If a child node yields a feasible integer solution for all time steps with a total cost lower than the current upper bound, update the best solution and the upper bound accordingly.
   - **Repeat**: Continue the loop until the priority queue is empty.

3. **Output**:
   - The best feasible solution found during the search is the optimal pump schedule, minimizing the total energy cost while satisfying all constraints.
   - Convert the $y_t$ values to individual pump schedules $x_{it}$ by sequentially activating pumps to minimize the total number of actuations.

#### Implementation Details

To implement the Branch-and-Bound algorithm with incremental bounding effectively, we focus on key steps to ensure both accuracy and efficiency:

- **Updating Initial Conditions for Each Time Step**:
  - **Reservoir Levels**: Carry over reservoir levels from the previous time step to serve as the starting levels for the current one.
  - **Pressure Heads**: Update pressure heads based on results from the previous time step.
  - **Pump Statuses**: Reflect any changes in pump on/off statuses in the system's current state.
  - **Time Parameters**: Set the start and end times according to the current scheduling period.

By customizing these initial conditions for each subproblem, we ensure that the EPANET simulator accurately represents the water distribution network at every time step.

- **Customized Problem Definitions for Child Nodes**:
  - In the Branch-and-Bound algorithm, each feasible parent node maintains a tailored EPANET problem setup for its child nodes.
  - This involves updating network pressures, reservoir levels, pump statuses, and time parameters based on the previous simulation results.
  - By doing this, each child node starts with the most current state of the network, improving the accuracy and feasibility of scheduling decisions.

Implementing these steps helps the algorithm make precise and reliable decisions for optimal pump scheduling.

 
#### Computational Complexity

The Branch-and-Bound algorithm has an exponential time complexity $ O(N^T) $ in the worst case, as it explores all possible combinations of $ y_t $ across all time steps. However, effective pruning and bounding can significantly reduce the number of subproblems examined. The efficiency of the B&B approach depends on the tightness of bounds, intelligent branching strategies, exploiting problem-specific structures, and the quality of initial feasible solutions. The incremental bounding approach provides tighter lower bounds, enhancing pruning effectiveness. Intelligent branching focuses on promising variables, while exploiting temporal dependencies and identical pump characteristics streamlines the search process. High-quality initial solutions set stringent upper bounds early, enabling more aggressive pruning. In our case study, simplifications like identical pumps and fixed time steps, combined with incremental bounding, reduce complexity and enhance the B&B algorithm's performance.

## Case Study: Any Town
The numerical experiments were conducted using the Any Town (modified) network, which is an expanded version of the network initially introduced by Walski et al. (1987) and later modified by Rao and Salomons (2007). The network consists of a source of supply, three pumps, and three storage tanks. The operational problem involves scheduling the pumps to minimize energy costs while satisfying constraints related to tank levels, node pressures, and the number of pump actions.

#### Network Configuration
- **Storage Tanks**: The maximum, minimum, and initial levels for the storage tanks are 71.53 meters, 66.53 meters, and 66.93 meters, respectively.
- **Node Pressure Constraints**: The minimum pressures required at critical nodes are 51 meters at node 90, 42 meters at node 50, and 30 meters at node 170.
- **Pump Actions**: The maximum number of allowable pump actions is set at three.

The demand patterns, constraints, and energy tariff schedules applied to the network are consistent with those utilized by Rao and Salomons (2007). Additionally, the algorithm was enhanced to prohibit the operation of all three pumps during peak tariff periods (18:00–21:00), to further reduce the search space and improve the algorithm's performance.

#### Simulation Details
The simulations were performed using EPANET as the hydraulic solver, with a time step of one hour for a total operational period of 24 hours. The optimization sought to minimize the total energy cost, subject to the constraints of maintaining the tank levels, ensuring node pressures remained within the specified limits, and limiting the number of pump actions.
