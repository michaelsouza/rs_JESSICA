START EPANET simulation

1. Read .inp file (load network configuration)
   - Load nodes, links, pumps, tanks, initial demands
   - Store initial values in MEMORY (pressures, flows, tank levels)

2. Initialize the hydraulic solver
   - Set initial time t = 0
   - Define time step interval (e.g., 30 minutes)

3. FOR each simulation step (up to 3 steps) DO:
   
   3.1. Calculate hydraulic conditions for the current step:
        - Use current MEMORY values (pressures, flows, demands, tank levels)
        - Apply mass balance and energy conservation equations:
            - Update pressures at each node
            - Update flows in each link
            - Adjust tank levels (inflow and outflow)
            - Check pump statuses (on/off)
        
   3.2. Store current step results in MEMORY:
        - Update MEMORY with newly calculated values:
            - Pressures at nodes
            - Flows in links
            - Tank levels
            - Pump statuses
        - These values will be used in the next time step

   3.3. Retrieve or save results (optional):
        - Print or save to .rpt file (optional):
            - Pressures at each node
            - Flows in each link
            - Tank levels
        - These values can also be accessed in real-time via functions like EN_getNodeValue() and EN_getLinkValue()

   3.4. Advance to the next time step:
        - t = t + Δt (increment time)
        - Use MEMORY values (updated from the previous step) to start the next calculation

END FOR

4. End the simulation
   - Write final report (.rpt or .out) if necessary
   - Clear memory and close the project

END EPANET simulation
