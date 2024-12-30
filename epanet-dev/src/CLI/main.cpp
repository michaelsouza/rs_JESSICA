#include "BBConfig.h"
#include "BBSolver.h"
#include "Profiler.h"

#include <mpi.h>
#include <string>
#include <unistd.h>
#include <vector>

int main(int argc, char *argv[])
{
  const int num_threads = 4;

  // Initialize config
  BBConfig config(argc, argv);

  // Initialize Projects and Constraints
  std::vector<Project> projects(num_threads);
  std::vector<BBConstraints> constraints;
  constraints.reserve(num_threads);
  for (int i = 0; i < num_threads; ++i)
  {
    constraints.emplace_back(config); // Explicit initialization
  }

  // Create priority queue of tasks sorted by min (cost / h_root)
  auto cmp = [](const BBTask &a, const BBTask &b) { return a.cost / a.h_root > b.cost / b.h_root; };
  BBTaskQueue tasks;

  // Initialize first task
  BBTask task;
  task.h_root = 0;
  task.y = std::vector<int>(config.h_max + 1, 0);
  task.cost = std::numeric_limits<double>::infinity();
  tasks.push(task);

  omp_set_num_threads(num_threads);
#pragma omp parallel
  {
    int tid = omp_get_thread_num();
    Project &p = projects[tid];
    BBConstraints &c = constraints[tid];
  }

  return EXIT_SUCCESS;
}
