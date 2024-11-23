// src/CLI/main.cpp

#include "BBSolver.h"
#include "BBSolverConfig.h"
#include "BBTests.h"

#include <mpi.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
  // Initialize MPI
  MPI_Init(nullptr, nullptr);

  // Run tests
  test_all();

  // Create configuration from command-line arguments
  BBSolverConfig config(argc, argv);

  // Create solver instance
  BBSolver solver(config);

  // Start solving
  solver.solve();

  // Finalize MPI
  MPI_Finalize();

  return EXIT_SUCCESS;
}