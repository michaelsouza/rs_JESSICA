// src/CLI/BBSolverConfig.cpp
#include "BBSolverConfig.h"
#include <string>

BBSolverConfig::BBSolverConfig(int argc, char *argv[])
{
  parse_args(argc, argv);
}

void BBSolverConfig::parse_args(int argc, char *argv[])
{
  // Default input file
  inpFile = "/home/michael/gitrepos/rs_JESSICA/networks/any-town.inp";

  // Parse command line arguments
  for (int i = 1; i < argc; ++i)
  {
    std::string arg = argv[i];
    if (arg == "-v" || arg == "--verbose")
      verbose = true;
    else if (arg == "-h" || arg == "--h_max")
      h_max = std::stoi(argv[++i]);
    else if (arg == "-a" || arg == "--max_actuations")
      max_actuations = std::stoi(argv[++i]);
    else if (arg == "-s" || arg == "--save")
      save_project = true;
    else if (arg == "-l" || arg == "--log")
      use_logger = true;
    else if (arg == "-t" || arg == "--h_threshold")
      h_threshold = std::stoi(argv[++i]);
    else
      inpFile = arg; // Assume the input file is the last argument
  }
}
