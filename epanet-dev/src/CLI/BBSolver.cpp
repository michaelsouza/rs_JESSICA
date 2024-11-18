// src/BBSolver.cpp

#include "Core/network.h"
#include "Core/project.h"
#include "Elements/pump.h"
#include "epanet3.h"

#include "BBCounter.h"
#include "BBSolver.h"
#include "BBStats.h"
#include "ColorStream.h" // Include ColorStream header
#include "Helper.h"
#include "Utils.h"

#include <algorithm>
#include <chrono>
#include <cstdlib> // For std::atoi
#include <cstring>
#include <fstream> // Required for checking file existence
#include <iostream>
#include <limits>
#include <map>
#include <numeric>
#include <string>
#include <vector>

int bbsolver(int argc, char *argv[]) {
  // Default values
  const char *inpFile =
      "/home/michael/gitrepos/rs_JESSICA/networks/any-town.inp";
  int h_max = 24;
  int max_actuations = 3;
  bool verbose = false;
  unsigned int maxiter =
      std::numeric_limits<unsigned int>::max(); // Default: unlimited

  // Command-line parameter parsing
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--h_max") == 0 && i + 1 < argc) {
      h_max = std::atoi(argv[++i]);
      if (h_max < 3 || h_max > 24) {
        std::cerr << "Error: h_max must be between 3 and 24." << std::endl;
        return EXIT_FAILURE;
      }
    } else if (std::strcmp(argv[i], "--max_actuations") == 0 && i + 1 < argc) {
      max_actuations = std::atoi(argv[++i]);
    } else if (std::strcmp(argv[i], "--verbose") == 0) {
      verbose = true;
    } else if (std::strcmp(argv[i], "--maxiter") == 0 && i + 1 < argc) {
      maxiter = std::atoi(argv[++i]);
      if (maxiter == 0) {
        std::cerr << "Error: maxiter must be greater than 0." << std::endl;
        return EXIT_FAILURE;
      }
    } else {
      std::cerr << "Unknown argument: " << argv[i] << std::endl;
      return EXIT_FAILURE;
    }
  }

  if (verbose) {
    printf("inpFile          %s\n", inpFile);
    printf("h_max            %d\n", h_max);
    printf("max_actuations   %d\n", max_actuations);
    printf("maxiter          %u\n", maxiter);
    printf("verbose          %s\n", verbose ? "true" : "false");
  }

  // Check if the inpFile exists
  std::ifstream fileCheck(inpFile);
  if (!fileCheck) {
    ColorStream::println("Error: Input file " + std::string(inpFile) +
                             " does not exist.",
                         ColorStream::Color::RED);
    return false;
  }

  // Initialize nodes and tanks with placeholder IDs
  std::map<std::string, int> nodes = {{"55", 0}, {"90", 0}, {"170", 0}};
  std::map<std::string, int> tanks = {{"65", 0}, {"165", 0}, {"265", 0}};
  std::vector<std::string> pump_names = {"111", "222", "333"};

  // Retrieve node and tank IDs from the input file
  get_nodes_and_tanks_ids(inpFile, nodes, tanks, verbose);

  // Display nodes, pumps, and tanks information
  show_nodes_pumps_tanks(nodes, pump_names, tanks, verbose);

  // Initialize branch-and-bound counter and statistics
  BBCounter counter(nodes.size(), h_max, max_actuations, pump_names.size());
  BBStats stats(h_max, max_actuations);

  bool is_feasible = true;
  unsigned int niter = 0;

  auto tic = std::chrono::high_resolution_clock::now();

  // Branch-and-bound loop
  while (counter.update_y(is_feasible)) {
    niter++;
    if (niter > maxiter) {
      ColorStream::println("\nMaximum iterations reached (" +
                               std::to_string(maxiter) + "). Terminating.",
                           ColorStream::Color::RED);
      break;
    }

    show_timer(niter, tic);

    is_feasible = counter.update_x(verbose);
    counter.show_xy(verbose);

    if (!is_feasible) {
      if (verbose) {
        printf("⚠️ actuations: %d >= %d\n", counter.y[counter.h],
               counter.max_actuations);
      }
      stats.record_pruning("actuations", counter.h);
      continue;
    }

    is_feasible = process_node(inpFile, counter, stats, nodes, tanks,
                               pump_names, verbose);
    if (verbose) {
      std::cout << "\nis_feasible: " << is_feasible << "\n" << std::endl;
    }
  }

  return EXIT_SUCCESS;
}
