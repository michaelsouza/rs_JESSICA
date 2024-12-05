// src/CLI/main.cpp

#include "BBConfig.h"
#include "BBSolver.h"
#include "BBTests.h"

#include <mpi.h>
#include <unistd.h>

#include <string>
#include <vector>

void run_tests(int argc, char *argv[])
{
  std::vector<std::string> test_names;
  bool run_tests_flag = false;

  // Search for --test parameter
  for (int i = 1; i < argc; ++i)
  {
    std::string arg = argv[i];
    if (arg == "--test")
    {
      run_tests_flag = true;
      // Collect all subsequent arguments as test names
      while (++i < argc)
      {
        std::string test_name = argv[i];
        if (test_name.rfind("--", 0) == 0)
        {
          // Encountered another flag, stop collecting test names
          i--;
          break;
        }
        test_names.push_back(test_name);
      }
    }
  }

  if (run_tests_flag)
  {
    if (test_names.empty())
    {
      Console::printf(Console::Color::BRIGHT_RED, "No test names provided\n");
      exit(EXIT_FAILURE);
    }
    else
      test_all(test_names);
  }
}

int main(int argc, char *argv[])
{
  run_tests(argc, argv);

  // Initialize MPI
  MPI_Init(nullptr, nullptr);

  // Create configuration from command-line arguments
  BBConfig config(argc, argv);
  config.show();

  // Create solver instance
  BBSolver solver(config);

  // Start solving
  solver.solve();

  // Finalize MPI
  MPI_Finalize();

  return EXIT_SUCCESS;
}