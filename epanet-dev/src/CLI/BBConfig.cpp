// src/CLI/BBConfig.cpp
#include "BBConfig.h"
#include "Console.h"

#include <mpi.h>
#include <omp.h>
#include <string>

BBConfig::BBConfig(int argc, char *argv[])
{
  // Default input file
  inpFile = "/home/michael/gitrepos/rs_JESSICA/networks/any-town.inp";

  // Parse command line arguments
  for (int i = 1; i < argc; ++i)
  {
    std::string arg = argv[i];
    if (arg == "-i" || arg == "--input")
      inpFile = argv[++i];
    else if (arg == "-v" || arg == "--verbose")
      verbose = true;
    else if (arg == "-h" || arg == "--h_max")
      h_max = std::stoi(argv[++i]);
    else if (arg == "-a" || arg == "--max_actuations")
      max_actuations = std::stoi(argv[++i]);
    else if (arg == "-n" || arg == "--num_threads")
      num_threads = std::stoi(argv[++i]);
    else if (arg == "-k" || arg == "--max_tasks")
      max_tasks = std::stoi(argv[++i]);
  }

  // Set the number of threads for subsequent parallel regions
  omp_set_num_threads(num_threads);

  // Verify the number of threads in a parallel region
  int actual_num_threads = 0;
#pragma omp parallel
  {
#pragma omp single
    {
      actual_num_threads = omp_get_num_threads();
    }
  }

  if (actual_num_threads != num_threads)
  {
    throw std::runtime_error("Number of threads mismatch: " + std::to_string(actual_num_threads) + " != " + std::to_string(num_threads));
  }

  std::string fn_stats_base = "run_h_" + std::to_string(h_max) + "_a_" + std::to_string(max_actuations) + "_n_" + std::to_string(num_threads) + "_k_" +
                              std::to_string(max_tasks);

  fn_stats = fn_stats_base + "_stats.json";
  fn_best = fn_stats_base + "_best.json";
}

void BBConfig::show() const
{
  Console::printf(Console::Color::CYAN, "════════════════════════════════════════\n");
  Console::printf(Console::Color::CYAN, "Branch & Bound Configuration:\n");
  Console::printf(Console::Color::WHITE, "  Input file:      %s\n", inpFile.c_str());
  Console::printf(Console::Color::WHITE, "  Max hours:       %d\n", h_max);
  Console::printf(Console::Color::WHITE, "  Max actuations:  %d\n", max_actuations);
  Console::printf(Console::Color::WHITE, "  Verbose:         %s\n", verbose ? "true" : "false");
  Console::printf(Console::Color::WHITE, "  Max tasks:       %d\n", max_tasks);
  Console::printf(Console::Color::WHITE, "  Num threads:     %d\n", num_threads);
  Console::printf(Console::Color::WHITE, "  Stats file:      %s\n", fn_stats.c_str());
  Console::printf(Console::Color::WHITE, "  Best file:       %s\n", fn_best.c_str());
}
