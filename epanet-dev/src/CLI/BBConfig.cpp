// src/CLI/BBConfig.cpp
#include "BBConfig.h"
#include "Console.h"

#include <mpi.h>
#include <string>

void BBConfig::generateFilenames()
{
  int rank, np;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &np);

  char fn_base[256];
  // Format the base filename
  int ret = snprintf(fn_base, sizeof(fn_base), "run_h_%02d_a_%02d_l_%02d_n_%02d_r_%02d", h_max, max_actuations, level, np, rank);
  if (ret < 0 || static_cast<size_t>(ret) >= sizeof(fn_base))
  {
    throw std::runtime_error("Filename truncation occurred in fn_base!");
  }

  // Format the stats filename
  ret = snprintf(fn_stats, sizeof(fn_stats), "%s_stats.json", fn_base);
  if (ret < 0 || static_cast<size_t>(ret) >= sizeof(fn_stats))
  {
    throw std::runtime_error("Filename truncation occurred in fn_stats!");
  }

  // Format the best filename
  ret = snprintf(fn_best, sizeof(fn_best), "%s_best.json", fn_base);
  if (ret < 0 || static_cast<size_t>(ret) >= sizeof(fn_best))
  {
    throw std::runtime_error("Filename truncation occurred in fn_best!");
  }

  // Format the profile filename
  ret = snprintf(fn_profile, sizeof(fn_profile), "%s_prof.txt", fn_base);
  if (ret < 0 || static_cast<size_t>(ret) >= sizeof(fn_profile))
  {
    throw std::runtime_error("Filename truncation occurred in fn_profile!");
  }
}

BBConfig::BBConfig(int argc, char *argv[])
{
  int rank, np;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &np);

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
    else if (arg == "-l" || arg == "--level")
      level = std::stoi(argv[++i]);
  }

  // Buffers for filenames
  try
  {
    generateFilenames();
  }
  catch (const std::runtime_error &e)
  {
    throw std::runtime_error(e.what());
  }
}

void BBConfig::show() const
{
  Console::printf(Console::Color::CYAN, "════════════════════════════════════════\n");
  Console::printf(Console::Color::CYAN, "Branch & Bound Configuration:\n");
  Console::printf(Console::Color::WHITE, "  Input file:      %s\n", inpFile.c_str());
  Console::printf(Console::Color::WHITE, "  Max hours:       %d\n", h_max);
  Console::printf(Console::Color::WHITE, "  Max actuations:  %d\n", max_actuations);
  Console::printf(Console::Color::WHITE, "  Level:           %d\n", level);
  Console::printf(Console::Color::WHITE, "  Verbose:         %s\n", verbose ? "true" : "false");
  Console::printf(Console::Color::WHITE, "  Stats file:      %s\n", fn_stats);
  Console::printf(Console::Color::WHITE, "  Best file:       %s\n", fn_best);
  Console::printf(Console::Color::WHITE, "  Profile file:    %s\n", fn_profile);
}
