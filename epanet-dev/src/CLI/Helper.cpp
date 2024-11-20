// src/CLI/Helper.cpp
#include "Helper.h"
#include <algorithm>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>

using Epanet::Project;

// Function to process a node in the branch-and-bound tree
bool process_node(const char *inpFile, BBCounter &counter, BBConstraints &cntrs, double &cost, bool verbose,
                  bool save_project)
{
  bool is_feasible = true;
  int t = 0, dt = 0, t_max = 3600 * counter.h;

  EN_Project p = EN_createProject();
  Project *prj = static_cast<Project *>(p);

  CHK(EN_loadProject(inpFile, p), "Load project");
  CHK(EN_initSolver(EN_INITFLOW, p), "Initialize solver");

  // Set the project and constraints
  cntrs.set_project(p, counter, verbose);

  do
  {
    // Run the solver
    CHK(EN_runSolver(&t, p), "Run solver");

    if (verbose)
    {
      printf("\nSimulation: t_max=%d, t: %d, dt: %d\n", t_max, t, dt);
    }

    // Check node pressures
    is_feasible = cntrs.check_pressures(verbose);
    if (!is_feasible)
    {
      counter.prune(PruneReason::PRESSURES);
      break;
    }

    // Check tank levels
    is_feasible = cntrs.check_levels(verbose);
    if (!is_feasible)
    {
      counter.prune(PruneReason::LEVELS);
      break;
    }

    // Check cost
    cost = cntrs.calc_cost();
    is_feasible = cntrs.check_cost(cost, verbose);
    if (!is_feasible)
    {
      counter.prune(PruneReason::COST);
      counter.jump_to_end();
      break;
    }

    // Advance the solver
    CHK(EN_advanceSolver(&dt, p), "Advance solver");

    // Check if we have reached the maximum simulation time
    if (t + dt > t_max)
    {
      break;
    }
  } while (dt > 0);

  // Check stability for the last hour
  if (is_feasible && counter.h == counter.h_max)
  {
    is_feasible = cntrs.check_stability(verbose);
    if (!is_feasible)
    {
      counter.prune(PruneReason::STABILITY);
    }
  }

  if (save_project)
  {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << "output_" << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S") << ".inp";
    prj->save(ss.str().c_str());
    ColorStream::printf(ColorStream::Color::BRIGHT_GREEN, "Project saved to: %s\n", ss.str().c_str());
  }

  // Delete the project
  EN_deleteProject(p);

  return is_feasible;
}
