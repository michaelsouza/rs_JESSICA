// src/CLI/main.cpp

#include "BBSolver.h"
#include "BBTests.h"

#include <mpi.h>

int main(int argc, char *argv[])
{
  MPI_Init(nullptr, nullptr);

  solve(argc, argv);
  // test_all();

  MPI_Finalize();
  return EXIT_SUCCESS;
}