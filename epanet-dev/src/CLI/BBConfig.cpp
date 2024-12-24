// src/CLI/BBConfig.cpp
#include "BBConfig.h"
#include "Console.h"
#include <mpi.h>
#include <string>

BBConfig::BBConfig(int argc, char *argv[])
{
  parse_args(argc, argv);
}

void BBConfig::parse_args(int argc, char *argv[])
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
    else if (arg == "-s" || arg == "--dump")
      dump_project = true;
    else if (arg == "-l" || arg == "--log")
      use_logger = true;
    else if (arg == "-t" || arg == "--h_threshold")
      h_threshold = std::stoi(argv[++i]);
  }
}

void BBConfig::show() const
{
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  if (rank == 0)
  {
    Console::printf(Console::Color::CYAN, "════════════════════════════════════════\n");
    Console::printf(Console::Color::CYAN, "Branch & Bound Configuration (Rank %d):\n", rank);
    Console::printf(Console::Color::WHITE, "  Input file:      %s\n", inpFile.c_str());
    Console::printf(Console::Color::WHITE, "  Max hours:       %d\n", h_max);
    Console::printf(Console::Color::WHITE, "  Max actuations:  %d\n", max_actuations);
    Console::printf(Console::Color::WHITE, "  Hour threshold:  %d\n", h_threshold);
    Console::printf(Console::Color::WHITE, "  Verbose:         %s\n", verbose ? "true" : "false");
    Console::printf(Console::Color::WHITE, "  Save project:    %s\n", dump_project ? "true" : "false");
    Console::printf(Console::Color::WHITE, "  Use logger:      %s\n", use_logger ? "true" : "false");
  }
}
