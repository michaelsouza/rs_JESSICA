// src/CLI/main.cpp

#include "BBSolver.h"
#include "BBConfig.h"
#include "BBTests.h"

#include <mpi.h>
#include <unistd.h>

void run_tests(int argc, char*argv[]){
  // Search for --test parameter
  for (int i = 1; i < argc; ++i)
  {
    std::string arg = argv[i];
    if (arg == "--test")
    {
      test_all();
    }
  }
}

int main(int argc, char *argv[])
{
  run_tests(argc, argv);  

  // Initialize MPI
  MPI_Init(nullptr, nullptr);

  // Create configuration from command-line arguments
  BBConfig config(argc, argv);

  // Create solver instance
  BBSolver solver(config);

  // Start solving
  solver.solve();

  // Finalize MPI
  MPI_Finalize();

  return EXIT_SUCCESS;
}