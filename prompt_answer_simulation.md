**Reframed Question:**

I'm using EPANET 3 to simulate water distribution systems and have successfully built and run the command-line executable as per the instructions in the README. While I can obtain the final simulation results in the report file, I need to access intermediate results at each simulation time step, such as node pressures, link flows, tank levels, and other parameters. How can I modify the existing code or utilize the EPANET 3 API to retrieve these intermediate results during the simulation?

**Answer:**

To access intermediate results during the EPANET 3 simulation, you can utilize the EPANET 3 API functions within the simulation loop to extract the desired parameters at each time step. Here's how you can modify your code to achieve this:

1. **Understand the Simulation Loop:**

   In your `main.cpp`, the simulation loop is structured using `EN_runSolver` and `EN_advanceSolver` functions:

   ```cpp
   do {
       EN_runSolver(&t, p);
       // Insert code here to retrieve intermediate results
       EN_advanceSolver(&dt, p);
   } while (dt > 0);
   ```

   - `EN_runSolver(&t, p);` runs the hydraulic and water quality simulation for the current time step and updates the simulation time `t`.
   - `EN_advanceSolver(&dt, p);` advances the simulation to the next event and updates the time step `dt`.

2. **Retrieve Intermediate Results:**

   Within the simulation loop, after `EN_runSolver` and before `EN_advanceSolver`, you can call functions like `EN_getNodeValue` and `EN_getLinkValue` to retrieve values for nodes and links at each time step.

   Here's how you can modify your loop:

   ```cpp
   do {
       EN_runSolver(&t, p);

       // Retrieve intermediate results
       int nodeCount, linkCount;
       EN_getCount(EN_NODECOUNT, &nodeCount, p);
       EN_getCount(EN_LINKCOUNT, &linkCount, p);

       // Loop through all nodes to get pressures
       for (int i = 1; i <= nodeCount; i++) {
           double pressure;
           EN_getNodeValue(i, EN_PRESSURE, &pressure, p);
           // Store or output the pressure value as needed
           std::cout << "Time: " << t << ", Node " << i << ", Pressure: " << pressure << std::endl;
       }

       // Loop through all links to get flows
       for (int i = 1; i <= linkCount; i++) {
           double flow;
           EN_getLinkValue(i, EN_FLOW, &flow, p);
           // Store or output the flow value as needed
           std::cout << "Time: " << t << ", Link " << i << ", Flow: " << flow << std::endl;
       }

       EN_advanceSolver(&dt, p);
   } while (dt > 0);
   ```

   - Replace `std::cout` statements with your preferred method of storing or processing the data.
   - Make sure to include the necessary headers and check for errors after each API call.

3. **Initialize and Finalize the Simulation Properly:**

   Ensure that you have properly initialized the simulation before entering the loop and that you close the solver after the loop:

   ```cpp
   // Initialize the simulation (use EN_INITFLOW if you want to use initial flows)
   EN_initSolver(EN_INITFLOW, p);

   // Simulation loop as shown above

   // After the loop, close the solver
   EN_closeSolver(p);
   ```

4. **Compile and Run:**

   After modifying your `main.cpp`, recompile the executable using CMake or your chosen build system:

   ```bash
   mkdir build && cd build
   cmake .. && make
   ```

   Then run your executable with the input and report files:

   ```bash
   ./run-epanet3 input.inp report.txt
   ```

5. **Consider Thread Safety (Optional):**

   If you plan to run multiple simulations in parallel, ensure that your implementation accounts for thread safety. EPANET 3's API allows for analyzing several projects in parallel in a thread-safe manner.

6. **Check the API Documentation:**

   Refer to the EPANET 3 API documentation for detailed information on the available functions and their usage:

   - `EN_getNodeValue(int index, int param, double *value, EN_Project p);`
   - `EN_getLinkValue(int index, int param, double *value, EN_Project p);`
   - The `param` argument accepts enumerations like `EN_PRESSURE`, `EN_FLOW`, etc.

   You can access the full documentation at [wateranalytics.org/epanet-dev](http://wateranalytics.org/epanet-dev).

7. **Handle Errors:**

   Always check the return values of API functions to handle errors appropriately. You can use `EN_getError` to get error messages corresponding to error codes.

**Example Code Snippet:**

Below is a simplified example of how your `main.cpp` might look after modifications:

```cpp
#include "epanet3.h"
#include <iostream>

int main(int argc, char* argv[])
{
    // Check command-line arguments
    if (argc < 3) {
        std::cout << "\nCorrect syntax is: epanet3 inpFile rptFile (outFile)\n";
        return 0;
    }

    const char* inpFile = argv[1];
    const char* rptFile = argv[2];
    const char* outFile = (argc > 3) ? argv[3] : "";

    int err, t = 0, dt = 0;
    EN_Project p = EN_createProject();
    err = EN_openReportFile(rptFile, p);
    err = EN_loadProject(inpFile, p);
    err = EN_openOutputFile(outFile, p);

    // Initialize the simulation
    err = EN_initSolver(EN_INITFLOW, p);

    // Simulation loop
    do {
        err = EN_runSolver(&t, p);

        // Retrieve intermediate results
        int nodeCount, linkCount;
        err = EN_getCount(EN_NODECOUNT, &nodeCount, p);
        err = EN_getCount(EN_LINKCOUNT, &linkCount, p);

        for (int i = 1; i <= nodeCount; i++) {
            double pressure;
            err = EN_getNodeValue(i, EN_PRESSURE, &pressure, p);
            std::cout << "Time: " << t << ", Node " << i << ", Pressure: " << pressure << std::endl;
        }

        for (int i = 1; i <= linkCount; i++) {
            double flow;
            err = EN_getLinkValue(i, EN_FLOW, &flow, p);
            std::cout << "Time: " << t << ", Link " << i << ", Flow: " << flow << std::endl;
        }

        err = EN_advanceSolver(&dt, p);
    } while (dt > 0);

    // Close the solver and write the report
    err = EN_closeSolver(p);
    err = EN_writeReport(p);
    EN_deleteProject(p);

    return 0;
}
```

**Notes:**

- Replace `std::cout` with file operations or data structures if you need to store the results for further analysis.
- Ensure that all API function calls check for errors (`err` variable) and handle them as needed.
- This example outputs pressures and flows at each time step. You can retrieve other parameters like tank levels, node demands, and water quality similarly by using the appropriate `EN_getNodeValue` or `EN_getLinkValue` parameters.

By incorporating these modifications, you'll be able to access and utilize intermediate simulation results during the EPANET 3 simulation process.