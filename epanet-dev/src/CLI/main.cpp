// src/CLI/main.cpp

#include "BBConfig.h"
#include "BBSolver.h"
#include "BBStatistics.h"
#include "Profiler.h"

#include <mpi.h>
#include <string>
#include <vector>

#include <algorithm>
#include <vector>

void populate_tasks(std::vector<BBTask> &tasks, const BBConfig &config, const BBConstraints &constraints)
{
  int rank, num_procs;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

  int level = std::min(config.h_max - 2, config.level);
  std::vector<int> y(config.h_max + 1, 0);

  int num_pumps = constraints.get_num_pumps();

  // Helper function to recursively generate all possible y vectors
  std::function<void(int)> generate_combinations = [&](int pos)
  {
    if (pos > level)
    {
      // Create and configure the task
      BBTask task;
      task.y = y;
      task.h_root = level + 1;
      task.num_pumps = num_pumps;
      tasks.push_back(std::move(task));
      return;
    }

    // Generate combinations for y[pos] in range [0, num_pumps]
    for (int pump_state = 0; pump_state <= num_pumps; ++pump_state)
    {
      y[pos] = pump_state;
      generate_combinations(pos + 1);
    }
  };

  // Start generating combinations from the first position
  generate_combinations(1); // Start at index 1 to skip y[0] (root level)

  for (size_t uid = 0; uid < tasks.size(); uid++)
  {
    // set the uid
    tasks[uid].uid = uid;
    tasks[uid].tid = uid % num_procs;
  }

  if (rank == 0)
  {
    Console::printf(Console::Color::BRIGHT_YELLOW, "Generated %d tasks\n", tasks.size());
  }
}

int main(int argc, char *argv[])
{
  MPI_Init(&argc, &argv);
  int rank, num_procs;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

  ProfileScope scope("main");

  // Parse config
  BBConfig config(argc, argv);
  BBConstraints constraints(config);
  BBStatistics stats(config);

  if (rank == 0) config.show();

  // Convert queue to vector for parallel processing
  std::vector<BBTask> tasks;
  populate_tasks(tasks, config, constraints);

  auto tic = std::chrono::high_resolution_clock::now();

  for (size_t i = 0; i < tasks.size(); i++)
  {
    // Skip tasks that are not assigned to this process
    if (tasks[i].tid != rank) continue;
    // Process the task
    processTask(tasks[i], config, constraints, stats);
  }

  // signal that this process is finished
  constraints.finished = true;

  // Wait for all processes to finish
  while (!constraints.all_finished)
  {
    constraints.sync_best();
  }

  auto toc = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(toc - tic);

  stats.duration = duration.count() / 1e6; // seconds
  stats.to_json(config.fn_stats);
  constraints.to_json(config.fn_best);
  Profiler::save(config.fn_profile);

  if (rank == 0)
  {
    Console::printf(Console::Color::BRIGHT_CYAN, "Total time: %.3f seconds\n", duration.count() / 1e6);
    stats.show();
    constraints.show_best();
  }

  MPI_Finalize();
  return EXIT_SUCCESS;
}
