// src/CLI/BBSolver.cpp
#include "BBConfig.h"
#include "BBConstraints.h"
#include "Console.h"
#include "Profiler.h"

#include "Core/network.h"
#include "Core/project.h"
#include "Elements/node.h"
#include "Elements/tank.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <mpi.h>
#include <numeric>
#include <omp.h>
#include <queue>
#include <stdexcept>
#include <thread>

class BBTask
{
public:
  int h_root; // the first hour that can be changed
  double cost;
  std::vector<ProjectData> snapshots;
  std::vector<int> y;
  std::vector<int> x;
  int h;
  bool is_feasible;
  int num_pumps;
  Project *p;
};

// Define comparator for BBTask priority queue
struct BBTaskComparator
{
  bool operator()(const BBTask &a, const BBTask &b)
  {
    return a.cost / a.h_root > b.cost / b.h_root;
  }
};

// Define priority queue type for BBTask
using BBTaskQueue = std::priority_queue<BBTask, std::vector<BBTask>, BBTaskComparator>;

bool switch_pumps_off(int *x_new, const std::vector<int> &pumps_sorted, const std::vector<int> &allowed_10, int &counter_10)
{
  for (const int pump_id : pumps_sorted)
  {
    if (counter_10 <= 0) break;

    if (x_new[pump_id] == 1)
    {
      if (allowed_10[pump_id] <= 0) return false; // Cannot turn off this pump

      x_new[pump_id] = 0; // Turn off the pump
      --counter_10;
    }
  }
  return counter_10 == 0;
}

void compute_allowed_switches(const int num_pumps, const int *x, int current_h, std::vector<int> &allowed_01, std::vector<int> &allowed_10)
{
  for (int pump_id = 0; pump_id < num_pumps; ++pump_id)
  {
    for (int i = 2; i < current_h; ++i)
    {
      int x_old = x[pump_id + num_pumps * (i - 1)];
      int x_new = x[pump_id + num_pumps * i];
      if (x_old < x_new)
      {
        // Pump was turned on (0 -> 1)
        --allowed_01[pump_id];
      }
      else if (x_old > x_new)
      {
        // Pump was turned off (1 -> 0)
        --allowed_10[pump_id];
      }
    }
  }
}

void sort_pumps(std::vector<int> &pumps_sorted, const std::vector<int> &allowed_01, const std::vector<int> &allowed_10, bool switch_on)
{
  if (switch_on)
  {
    // Sort pumps by decreasing (allowed_01, allowed_10)
    std::sort(pumps_sorted.begin(), pumps_sorted.end(),
              [&allowed_01, &allowed_10](int a, int b)
              {
                if (allowed_01[a] != allowed_01[b])
                {
                  return allowed_01[a] > allowed_01[b];
                }
                return allowed_10[a] > allowed_10[b];
              });
  }
  else
  {
    // Sort pumps by decreasing (allowed_10, allowed_01)
    std::sort(pumps_sorted.begin(), pumps_sorted.end(),
              [&allowed_01, &allowed_10](int a, int b)
              {
                if (allowed_10[a] != allowed_10[b])
                {
                  return allowed_10[a] > allowed_10[b];
                }
                return allowed_01[a] > allowed_01[b];
              });
  }
}

bool switch_pumps_on(int *x_new, const std::vector<int> &pumps_sorted, const std::vector<int> &allowed_01, int &counter_01)
{
  for (const int pump_id : pumps_sorted)
  {
    if (counter_01 <= 0) break;

    if (x_new[pump_id] == 0)
    {
      if (allowed_01[pump_id] <= 0) return false; // Cannot turn on this pump

      x_new[pump_id] = 1; // Turn on the pump
      --counter_01;
    }
  }
  return counter_01 == 0;
}

void update_cost(BBConstraints &constraints, double &cost)
{
#pragma omp atomic write
  constraints.cost_ub = cost;
}

void epanet_solve(BBTask &task, BBConfig &config, BBConstraints &constraints)
{
  const int t_min = 3600 * (task.h - 1);
  const int t_max = 3600 * task.h;
  PruneType prune_type = PruneType::PRUNE_NONE;
  Project &p = *(task.p);
  // Run the solver
  int t = 0, dt = 0;
  do
  {
    // Run the solver
    CHK(p.runSolver(&t), "Run solver");

    // Advance the solver
    CHK(p.advanceSolver(&dt), "Advance solver");

    const int t_new = t + dt;

    // Show the current state
    if (config.verbose)
    {
      Console::printf(Console::Color::CYAN, "===========================================");
      Console::printf(Console::Color::MAGENTA, "\nSimulation: t_new=%d, t_max=%d, t=%d, dt=%d\n", t_new, 3600 * task.h, t, dt);
    }

    // Check feasibility
    prune_type = constraints.check_feasibility(p, task.h, task.cost);
    task.is_feasible = prune_type == PruneType::PRUNE_NONE;

    // If the solution is not feasible, jump to the end of the level
    if (!task.is_feasible)
    {
      if (prune_type == PruneType::PRUNE_COST)
      {
        // jump to the end of the level
        task.y[task.h] = task.num_pumps;
      }
      return;
    };

    // When the simulation reaches t_max and it is not the last hour, break.
    // When it is the last hour, just stop if "dt == 0".
    if (t_new == t_max && task.h != config.h_max) break;
  } while (dt > 0);

  // Check stability if the the solution is feasible and the current hour is the last hour
  if (task.is_feasible && task.h == config.h_max)
  {
    task.is_feasible = constraints.check_stability(p, config.verbose);

    // Update cost_ub
    if (task.is_feasible) update_cost(constraints, task.cost);
  }
}

bool init_snapshots(BBTask &task, BBConfig &config, BBConstraints &constraints)
{
  Project &p = *(task.p);
  // load the project
  p.load(config.inpFile.c_str());
  Network *nw = p.getNetwork();
  int t_max = 3600 * config.h_max;
  nw->options.setOption(Options::TimeOption::TOTAL_DURATION, t_max);
  p.initSolver(EN_INITFLOW);

  // take snapshots
  task.snapshots.resize(config.h_max + 1);
  p.copy_to(task.snapshots[0]); // first snapshot is the initial state

  // run the solver
  int t, dt, t_new;
  int h = 0;
  do
  {
    p.runSolver(&t);
    p.advanceSolver(&dt);

    if (!constraints.check_feasibility(p, h, task.cost))
    {
      return false;
    }

    t_new = t + dt;

    // take a snapshot
    if (t_new % 3600 == 0)
    {
      h = t / 3600;
      p.copy_to(task.snapshots[h]);
    }

  } while (t_new < task.h_root);

  return true;
}

bool init_task(BBTask &task, BBTaskQueue &tasks, BBConfig &config, BBConstraints &constraints)
{
  // check if the task is large enough to be split
  if (task.h_root > config.h_min)
  {
    return false;
  }

  // there are already enough tasks
  if (tasks.size() > config.num_threads)
  {
    return false;
  }

  // get the number of pumps
  task.num_pumps = constraints.get_num_pumps();

  // split the task into two new tasks
  for (int i = 1; i <= task.num_pumps; ++i)
  {
    BBTask task_new;
    task_new.h_root = task.h_root + 1;
    task_new.y = task.y;
    task_new.y[task_new.h_root] = i;
    tasks.push(task_new);
  }

  // add the root task to the queue
  task.h_root = task.h_root + 1;
  task.y[task.h_root] = 0;

  init_snapshots(task, config, constraints);
  return true;
}

void update_y(BBTask &task, BBConfig &config)
{
  // Check if the current level is valid
  if (task.h > config.h_max) throw std::runtime_error("task.h > config.h_max");

  // There is no more work to do
  if (task.h < task.h_root)
  {
    task.is_feasible = false;
    return;
  }

  // If the current level is feasible =========================================
  if (task.is_feasible)
  {
    // If the current level is not the last level, move to the next level and reset its value
    if (task.h < config.h_max)
    {
      task.y[++task.h] = 0;
      task.is_feasible = true;
      return;
    }

    // task.h == config.h_max
    if (task.y[task.h] < task.num_pumps)
    {
      task.y[task.h]++;
      task.is_feasible = true;
      return;
    }

    // Otherwise, decrement the current level
    --task.h;
    task.is_feasible = false;
    return update_y(task, config);
  }

  // The level is unfeasible ==================================================
  if (task.h == task.h_root)
  {
    // Increment the current level
    if (task.y[task.h] < task.num_pumps)
    {
      task.y[task.h]++;
      task.is_feasible = true;
      return;
    }

    // There is no more work to do
    task.is_feasible = false;
    return;
  }

  if (task.h <= config.h_max)
  {
    // The level is finished
    if (task.y[task.h] == task.num_pumps)
    {
      --task.h;
      return update_y(task, config);
    }

    // Otherwise, increment the current level
    task.y[task.h]++;
    task.is_feasible = true;
    return;
  }
}

void update_x(BBTask &task, BBConfig &config)
{
  const bool verbose = config.verbose;
  int tid = omp_get_thread_num();

  // Get the previous and new states
  const int &y_old = task.y[task.h - 1];
  const int &y_new = task.y[task.h];
  const int *x_old = &task.x[task.num_pumps * (task.h - 1)];
  int *x_new = &task.x[task.num_pumps * task.h];

  // Start by copying the previous state
  std::copy(x_old, x_old + task.num_pumps, x_new);

  if (verbose) Console::printf(Console::Color::BRIGHT_MAGENTA, "TID[%d]: update_x_h[%d]: y_new=%d, y_old=%d\n", tid, task.h, y_new, y_old);

  // Nothing to be done
  if (y_new == y_old)
  {
    task.is_feasible = true;
    return;
  }

  // Initialize allowed switches
  std::vector<int> allowed_01(task.num_pumps, config.max_actuations);
  std::vector<int> allowed_10(task.num_pumps, config.max_actuations);

  // Compute allowed switches based on history
  compute_allowed_switches(task.num_pumps, &task.x[0], task.h, allowed_01, allowed_10);

  // Create and initialize pump indices
  std::vector<int> pumps_sorted(task.num_pumps);
  for (int pump_id = 0; pump_id < task.num_pumps; ++pump_id)
  {
    pumps_sorted[pump_id] = pump_id;
  }

  bool success = true;
  if (y_new > y_old) // Switch on pumps
  {
    sort_pumps(pumps_sorted, allowed_01, allowed_10, true);
    int counter_01 = y_new - y_old;
    success = switch_pumps_on(x_new, pumps_sorted, allowed_01, counter_01);
  }
  else if (y_new < y_old) // Switch off pumps
  {
    sort_pumps(pumps_sorted, allowed_01, allowed_10, false);
    int counter_10 = y_old - y_new;
    success = switch_pumps_off(x_new, pumps_sorted, allowed_10, counter_10);
  }

  if (verbose) show_vector(x_new, task.num_pumps, "   x_new");
  task.is_feasible = success;
}

bool set_y(BBTask &task, BBConfig &config, const std::vector<int> &y)
{
  // Copy y to the internal y vector
  task.y = y;

  // Start assuming the y vector is feasible
  task.is_feasible = true;

  // Set y and update x for all hours
  task.h = 0;
  for (int i = 0; i < config.h_max; ++i)
  {
    task.h++;
    update_x(task, config);
    if (!task.is_feasible) return false;
  }

  return true;
}

void update_pumps(BBTask &task, BBConfig &config, BBConstraints &constraints, bool full_update)
{
  Project &p = *(task.p);
  if (full_update)
  {
    // Update pumps for all time periods
    for (int i = 0; i <= config.h_max; ++i)
    {
      constraints.update_pumps(p, i, task.x, config.verbose);
    }
  }
  else
  {
    // Update pumps only for current time period
    constraints.update_pumps(p, task.h, task.x, config.verbose);
  }
}

void process_level(BBTask &task, BBConfig &config, BBConstraints &constraints)
{
  ProfileScope scope("process_node");

  // Load the previous snapshot
  {
    ProfileScope scope("copy_from");
    // p.from_json(snapshots[h - 1]);
    task.p->copy_from(task.snapshots[task.h - 1]);
  }

  // Set the project and constraints
  update_pumps(task, config, constraints, true);

  // Run simulation
  epanet_solve(task, config, constraints);

  // Take a snapshot if the solution is feasible
  if (task.is_feasible)
  {
    ProfileScope scope("copy_to");
    // snapshots[h] = p.to_json();
    task.p->copy_to(task.snapshots[task.h]);
  }
}

void solve_task(BBTask &task, BBConfig &config, BBConstraints &constraints)
{
  while (true)
  {
    // Update y vector
    update_y(task, config);
    if (!task.is_feasible)
    {
      constraints.add_prune(PruneType::PRUNE_ACTUATIONS, task.h);
      break;
    }

    // Update x vector
    update_x(task, config);
    if (!task.is_feasible)
    {
      constraints.add_prune(PruneType::PRUNE_ACTUATIONS, task.h);
      return;
    }

    // Process node
    process_level(task, config, constraints);
  }
}
