// BBSolver.h
#pragma once

#include "BBConfig.h"
#include "BBConstraints.h"
#include "BBStatistics.h"
#include "Console.h"
#include "Profiler.h"

#include "Core/project.h"
#include "Elements/node.h"
#include "Elements/tank.h"

#include <algorithm>
#include <queue>
#include <stdexcept>
#include <vector>

// Forward declarations
class BBTask;
class BBSolver;

//---------------------------------------------------------------------
// BBTask: Holds the data needed to process a single branch-and-bound task.
//---------------------------------------------------------------------
class BBTask
{
public:
  int uid;
  int h_root; // first hour that can be changed
  double cost;
  std::vector<ProjectData> snapshots;
  std::vector<int> y;
  std::vector<int> x;
  int h;
  bool is_feasible;
  int num_pumps;
  Project *p;
  int tid;

  double priority() const
  {
    return h_root;
  }

  BBTask() = default;
  BBTask(int uid, const BBConfig &config, const BBConstraints &constraints)
  {
    h_root = 1;
    y.resize(config.h_max + 1, 0);
    num_pumps = constraints.get_num_pumps();
    this->uid = uid;
  }

  bool operator<(const BBTask &other) const
  {
    return priority() > other.priority(); // Higher priority comes first
  }

  void show() const
  {
    Console::printf(Console::Color::BRIGHT_YELLOW, "uid=%d, h_root=%d\n", uid, h_root);
    show_vector(y, "y");
  }

  void show_xy(int h, bool all = false) const
  {
    if (!all) // show only for the current hour
    {
      Console::printf(Console::Color::BRIGHT_YELLOW, "TID[%d]: h=%d, y=%d, x=[ ", tid, h, y[h]);
      for (int j = 0; j < num_pumps; ++j)
      {
        Console::printf(Console::Color::BRIGHT_YELLOW, "%d ", x[h * num_pumps + j]);
      }
      Console::printf(Console::Color::BRIGHT_YELLOW, "]\n");
    }
    else // show for all hours
    {
      for (int h = 0; h <= this->h; ++h)
        show_xy(h);
    }
  }

  void show_xy(bool all = false) const
  {
    show_xy(h, all);
  }
};

//---------------------------------------------------------------------
// BBPumpController: Manages pump switching logic.
//---------------------------------------------------------------------
class BBPumpController
{
public:
  static bool switchPumpsOff(int *x_new, const std::vector<int> &pumps_sorted, const std::vector<int> &allowed_10, int &counter_10)
  {
    for (int pump_id : pumps_sorted)
    {
      if (counter_10 <= 0) break;
      if (x_new[pump_id] == 1)
      {
        if (allowed_10[pump_id] <= 0) return false;
        x_new[pump_id] = 0;
        --counter_10;
      }
    }
    return (counter_10 == 0);
  }

  static bool switchPumpsOn(int *x_new, const std::vector<int> &pumps_sorted, const std::vector<int> &allowed_01, int &counter_01)
  {
    for (int pump_id : pumps_sorted)
    {
      if (counter_01 <= 0) break;
      if (x_new[pump_id] == 0)
      {
        if (allowed_01[pump_id] <= 0) return false;
        x_new[pump_id] = 1;
        --counter_01;
      }
    }
    return (counter_01 == 0);
  }

  static void computeAllowedSwitches(int num_pumps, const int *x, int current_h, std::vector<int> &allowed_01, std::vector<int> &allowed_10)
  {
    for (int pump_id = 0; pump_id < num_pumps; ++pump_id)
    {
      for (int i = 2; i < current_h; ++i)
      {
        int x_old = x[pump_id + num_pumps * (i - 1)];
        int x_new = x[pump_id + num_pumps * i];
        if (x_old < x_new)
          --allowed_01[pump_id]; // 0 -> 1
        else if (x_old > x_new)
          --allowed_10[pump_id]; // 1 -> 0
      }
    }
  }

  static void sortPumps(std::vector<int> &pumps_sorted, const std::vector<int> &allowed_01, const std::vector<int> &allowed_10, bool switch_on)
  {
    if (switch_on)
    {
      // Sort by decreasing (allowed_01, then allowed_10)
      std::sort(pumps_sorted.begin(), pumps_sorted.end(),
                [&allowed_01, &allowed_10](int a, int b)
                {
                  if (allowed_01[a] != allowed_01[b]) return allowed_01[a] > allowed_01[b];
                  return allowed_10[a] > allowed_10[b];
                });
    }
    else
    {
      // Sort by decreasing (allowed_10, then allowed_01)
      std::sort(pumps_sorted.begin(), pumps_sorted.end(),
                [&allowed_01, &allowed_10](int a, int b)
                {
                  if (allowed_10[a] != allowed_10[b]) return allowed_10[a] > allowed_10[b];
                  return allowed_01[a] > allowed_01[b];
                });
    }
  }
};

//---------------------------------------------------------------------
// BBSolver: Main solver class that uses BBTask, BBPumpController, etc.
//---------------------------------------------------------------------
class BBSolver
{
public:
  // Constructor can take config and constraints references
  BBSolver(BBConfig &configRef, BBConstraints &constraintsRef, BBStatistics &statsRef)
      : config(configRef), constraints(constraintsRef), stats(statsRef)
  {
  }

  // Orchestrates the BBTask solution process
  void solveTask(BBTask &task)
  {
    Project p;
    task.p = &p;
    task.cost = std::numeric_limits<double>::max();
    task.x.resize((config.h_max + 1) * task.num_pumps, 0);
    task.is_feasible = true;

    // initialize snapshots
    BBPruneReason prune_reason = initSnapshots(task);
    if (prune_reason != BBPruneReason::NONE)
    {
      if (config.verbose) stats.show();
      return;
    }

    // branch-and-bound loop
    while (true)
    {
      updateY(task);

      // work is done if not feasible
      if (!task.is_feasible) break;

      updateX(task);
      if (!task.is_feasible)
      {
        stats.add_stats(BBPruneReason::ACTUATIONS, task.h);
        continue;
      }

      if (config.verbose)
      {
        Console::hline(Console::Color::BRIGHT_YELLOW, 20);
        Console::printf(Console::Color::BRIGHT_YELLOW, "TID[%d]: solveTask: h=%d\n", task.tid, task.h);
        task.show_xy(true);
      }

      // Process the current level
      stats.add_stats(processLevel(task), task.h);

      if (config.verbose) stats.show();
    }
  }

private:
  int niters = 0;
  BBConfig &config;
  BBConstraints &constraints;
  BBStatistics &stats;
  //---------------------------------------------------------------------
  // Helper function for tasks initialization
  //---------------------------------------------------------------------
  inline BBPruneReason initSnapshots(BBTask &task)
  {
    if (config.verbose)
    {
      Console::hline(Console::Color::BRIGHT_YELLOW, 20);
      Console::printf(Console::Color::BRIGHT_YELLOW, "TID[%d]: initSnapshots: task.h_root=%d\n", task.tid, task.h_root);
    }

    // load project
    Project &p = *(task.p);
    p.load(config.inpFile.c_str());
    Network *nw = p.getNetwork();
    int t_max = 3600 * config.h_max;
    nw->options.setOption(Options::TimeOption::TOTAL_DURATION, t_max);
    p.initSolver(EN_INITFLOW);

    // Initialize pumps
    for (int i = 1; i < task.h_root; ++i)
    {
      task.h = i;
      updateX(task);
      if (!task.is_feasible) return BBPruneReason::ACTUATIONS;
      updatePumps(task, false);
    }

    // copy snapshots
    task.snapshots.resize(config.h_max + 1);
    p.copy_to(task.snapshots[0]);

    // run solver to copy snapshots
    int t, dt, t_new;
    task.h = 0;

    // run solver to copy snapshots
    BBPruneReason prune_reason = BBPruneReason::NONE;
    do
    {
      CHK(p.runSolver(&t), "Run solver");
      CHK(p.advanceSolver(&dt), "Advance solver");
      t_new = t + dt;

      if (config.verbose)
      {
        Console::printf(Console::Color::MAGENTA, "\nTID[%d]: t_new=%d, t_max=%d, t=%d, dt=%d\n", task.tid, t_new, t_max, t, dt);
        if (config.verbose)
        {
          task.show_xy(task.h + 1);
        }
      }

      prune_reason = constraints.check_feasibility(p, task.h, task.cost, config.verbose);
      if (prune_reason != BBPruneReason::NONE)
      {
        stats.add_stats(prune_reason, task.h + 1);
        return prune_reason;
      }

      if (t_new % 3600 == 0)
      {
        task.h = t_new / 3600; // update hour
        p.copy_to(task.snapshots[task.h]);
      }
    } while (task.h < (task.h_root - 1)); // initialize the snapshots for hours up to h_root

    return prune_reason;
  }

  //===============================================================
  // 1) Moves to the next feasible y value or stops if none exist
  //===============================================================
  void updateY(BBTask &task)
  {
    if (task.h > config.h_max) throw std::runtime_error("task.h > config.h_max");

    // If current level is feasible
    if (task.is_feasible)
    {
      // If not the last level, move to the next
      if (task.h < config.h_max)
      {
        task.y[++task.h] = 0;
        task.is_feasible = true;
        return;
      }
      // If last level, increment if possible
      if (task.y[task.h] < task.num_pumps)
      {
        task.y[task.h]++;
        task.is_feasible = true;
        return;
      }
      // No more increments, backtrack
      --task.h;
      task.is_feasible = false;
      updateY(task);
      return;
    }
    else // Not feasible
    {
      if (task.h == task.h_root)
      {
        // Try incrementing at root
        if (task.y[task.h] < task.num_pumps)
        {
          task.y[task.h]++;
          task.is_feasible = true;
          return;
        }
        // No more options
        task.is_feasible = false;
        return;
      }
      if (task.h <= config.h_max)
      {
        if (task.y[task.h] == task.num_pumps)
        {
          --task.h;
          updateY(task);
          return;
        }
        // Try increment
        task.y[task.h]++;
        task.is_feasible = true;
        return;
      }
    }
  }

  //===============================================================
  // 2) Updates x based on changes in y
  //===============================================================
  void updateX(BBTask &task)
  {
    const bool verbose = config.verbose;

    // Ensure task.h is valid
    if (task.h < 1 || task.h > config.h_max)
      throw std::runtime_error("Invalid task.h=" + std::to_string(task.h) + " out of range [1, config.h_max=" + std::to_string(config.h_max) + "].");

    // Get the previous and new y
    const int &y_old = task.y[task.h - 1];
    const int &y_new = task.y[task.h];

    const int *x_old = &task.x[task.num_pumps * (task.h - 1)];
    int *x_new = &task.x[task.num_pumps * task.h];

    // Start by copying old state
    std::copy(x_old, x_old + task.num_pumps, x_new);

    // If no change
    if (y_new == y_old)
    {
      task.is_feasible = true;
      return;
    }

    // Initialize counters
    std::vector<int> allowed_01(task.num_pumps, config.max_actuations);
    std::vector<int> allowed_10(task.num_pumps, config.max_actuations);

    // Compute allowed switches
    BBPumpController::computeAllowedSwitches(task.num_pumps, &task.x[0], task.h, allowed_01, allowed_10);

    // Sort pumps
    std::vector<int> pumps_sorted(task.num_pumps);
    for (int pump_id = 0; pump_id < task.num_pumps; ++pump_id)
      pumps_sorted[pump_id] = pump_id;

    bool success = true;
    if (y_new > y_old)
    {
      int counter_01 = y_new - y_old;
      BBPumpController::sortPumps(pumps_sorted, allowed_01, allowed_10, true);
      success = BBPumpController::switchPumpsOn(x_new, pumps_sorted, allowed_01, counter_01);
    }
    else // y_new < y_old
    {
      int counter_10 = y_old - y_new;
      BBPumpController::sortPumps(pumps_sorted, allowed_01, allowed_10, false);
      success = BBPumpController::switchPumpsOff(x_new, pumps_sorted, allowed_10, counter_10);
    }

    task.is_feasible = success;
  }

  //===============================================================
  // 3) Processes the current level: loads a snapshot, updates pumps, runs sim
  //===============================================================
  BBPruneReason processLevel(BBTask &task)
  {
    if (config.verbose)
    {
      Console::hline(Console::Color::BRIGHT_YELLOW, 20);
      Console::printf(Console::Color::BRIGHT_YELLOW, "TID[%d]: processLevel: h=%d\n", task.tid, task.h);
    }

    // load previous state
    task.p->copy_from(task.snapshots[task.h - 1]);

    updatePumps(task, false);
    BBPruneReason prune_reason = epanetSolve(task);

    // copy current state to snapshot
    if (task.is_feasible) task.p->copy_to(task.snapshots[task.h]);

    return prune_reason;
  }

  //===============================================================
  // 4) Updates pumps in the Project according to x
  //===============================================================
  void updatePumps(BBTask &task, bool full_update)
  {
    Project &p = *(task.p);
    if (full_update)
    {
      for (int i = 0; i <= task.h; ++i)
        constraints.update_pumps(p, i, task.x, config.verbose);
    }
    else
    {
      constraints.update_pumps(p, task.h, task.x, config.verbose);
    }
  }

  //===============================================================
  // 5) Wrapper for running the solver on the Project
  //===============================================================
  BBPruneReason epanetSolve(BBTask &task)
  {
    ProfileScope scope("epanetSolve");

    const int t_min = 3600 * (task.h - 1);
    const int t_max = 3600 * task.h;
    BBPruneReason prune_reason = BBPruneReason::NONE;
    Project &p = *(task.p);

    int t = 0, dt = 0, t_new = t_min;
    do
    {
      CHK(p.runSolver(&t), "Run solver");
      CHK(p.advanceSolver(&dt), "Advance solver");

      t_new = t + dt;

      if (config.verbose)
      {
        int h = std::min(t_new / 3600 + 1, task.h);
        Console::printf(Console::Color::MAGENTA, "\nSimulation: t_min=%d <= t_new=%d <= t_max=%d, dt=%d\n", t_min, t_new, t_max, dt);
        task.show_xy(h);
      }

      // check feasibility
      prune_reason = constraints.check_feasibility(p, task.h, task.cost, config.verbose);

      task.is_feasible = (prune_reason == BBPruneReason::NONE);
      if (!task.is_feasible)
      {
        if (prune_reason == BBPruneReason::COST) task.y[task.h] = task.num_pumps; // jump to end
        return prune_reason;
      }

      // check if we are at the end of the simulation
      if (t_new == t_max && task.h != config.h_max) break;
    } while (dt > 0);

    // Check stability if last hour
    if (task.is_feasible && task.h == config.h_max)
    {
      prune_reason = constraints.check_stability(p, config.verbose);
      if (prune_reason != BBPruneReason::NONE) return prune_reason;

      if (task.is_feasible)
      {
        if (config.verbose)
        {
          // Format cost_ub
          char fmt_cost_ub[100];
          if (constraints.best_cost_local == std::numeric_limits<double>::max())
            snprintf(fmt_cost_ub, sizeof(fmt_cost_ub), "inf");
          else
            snprintf(fmt_cost_ub, sizeof(fmt_cost_ub), "%.2f", constraints.best_cost_local);
          // Show old and new cost
          Console::printf(Console::Color::BRIGHT_GREEN, "TID[%d]: cost update: 💰 cost=%.2f, cost_ub=%s\n", task.tid, task.cost, fmt_cost_ub);
        }

        // update best solution
        constraints.update_best(task.cost, task.x, task.y);
      }
    }

    return prune_reason;
  }
};

void processTask(BBTask &task, BBConfig &config, BBConstraints &constraints, BBStatistics &stats)
{
  ProfileScope scope("processTask");
  BBSolver solver(config, constraints, stats);
  solver.solveTask(task);
}
