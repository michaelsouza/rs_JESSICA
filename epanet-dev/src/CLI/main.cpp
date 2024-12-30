// src/CLI/main.cpp

#include "BBConfig.h"
#include "BBSolver.h"
#include "BBStatistics.h"
#include "Profiler.h"

#include <mpi.h>
#include <string>
#include <unistd.h>
#include <vector>

#include "BBConfig.h"
#include "BBSolver.h"
#include <algorithm>
#include <iostream>
#include <limits>
#include <omp.h>
#include <queue>
#include <vector>

void populate_tasks(BBTaskQueue &tasks, const BBConfig &config, const BBConstraints &constraints)
{
  // Populate the task queue with initial tasks
  int uid = 0;
  while (tasks.size() < config.max_tasks)
  {
    if (tasks.empty()) tasks.push(BBTask(uid++, config, constraints));

    // pop and split the task
    BBTask task;
    if (!tasks.pop(task)) break;

    // Check if the task is too small
    if (task.h_root > config.h_max - 2)
    {
      // If the task is too small, push it back to the queue and break
      tasks.push(task);
      break;
    }

    // Split the task into multiple tasks
    for (int num_active_pumps = 0; num_active_pumps <= task.num_pumps; num_active_pumps++)
    {
      BBTask new_task = task;
      new_task.uid = uid++;
      new_task.y[task.h_root] = num_active_pumps;
      ++new_task.h_root;
      tasks.push(new_task);
    }
  }

  if (config.verbose) Console::printf(Console::Color::BRIGHT_GREEN, "Tasks: %d\n", tasks.size());
}

int main(int argc, char *argv[])
{
  // Parse config
  BBConfig config(argc, argv);
  config.show();

  BBConstraints constraints(config);

  // Initialize statistics for each thread to avoid race condition
  std::vector<BBStatistics> stats(config.num_threads, BBStatistics(config));

  BBTaskQueue tasks;
  populate_tasks(tasks, config, constraints);

  auto tic = std::chrono::high_resolution_clock::now();
#pragma omp parallel
  {
    while (true)
    {
      BBTask task;
      if (!tasks.pop(task)) break;
      int tid = omp_get_thread_num();
      processTask(task, config, constraints, stats[tid]);
    }
  }
  auto toc = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(toc - tic);
  Console::printf(Console::Color::BRIGHT_CYAN, "Total time: %.3f seconds\n", duration.count() / 1e6);

  // Collect statistics from all threads
  BBStatistics total_stats(config);
  for (int tid = 0; tid < config.num_threads; ++tid)
    total_stats.merge(stats[tid]);

  total_stats.duration = duration.count() / 1e6; // seconds

  total_stats.show();
  total_stats.to_json(config.fn_stats);

  // Write best solution to file
  constraints.show_best();
  constraints.to_json(config.fn_best);

  return EXIT_SUCCESS;
}
