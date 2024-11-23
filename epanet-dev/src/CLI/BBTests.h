// BBTests.h

#pragma once

#include "BBConstraints.h"
#include "BBSolver.h"
#include "Console.h"
#include "Core/network.h"
#include "Core/project.h"
#include "Elements/pump.h"
#include "Utils.h"
#include "epanet3.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <mpi.h>
#include <numeric>
#include <string>
#include <thread>
#include <vector>

class BBTest
{
protected:
  std::string test_name;

public:
  bool verbose = false;

  virtual ~BBTest()
  {
  }

  virtual bool run() = 0;

  virtual void set_up()
  {
  }

  virtual void tear_down()
  {
  }

  virtual void print_test_name()
  {
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0)
    {
      Console::printf(Console::Color::BRIGHT_YELLOW, "Running %s...\n", test_name.c_str());
    }
  }

  virtual bool verify_cost(double cost, double expected_cost, double tolerance)
  {
    if (std::abs(cost - expected_cost) > tolerance)
    {
      Console::printf(Console::Color::RED, "Test Failed (%s): cost (%.2f) is not within %.2f of expected %.2f.\n", test_name.c_str(), cost, tolerance,
                      expected_cost);
      return false;
    }
    else
    {
      Console::printf(Console::Color::GREEN, "Test Passed (%s): cost (%.2f) is within %.2f of expected %.2f.\n", test_name.c_str(), cost, tolerance,
                      expected_cost);
      return true;
    }
  }
};

class TestCostBase : public BBTest
{
protected:
  std::vector<int> y;
  double expected_cost;
  double tolerance;

public:
  TestCostBase(const std::vector<int> &y, double expected_cost, double tolerance, const std::string &test_name)
      : y(y), expected_cost(expected_cost), tolerance(tolerance)
  {
    this->test_name = test_name;
  }

  bool run() override
  {
    set_up();
    return execute_test();
  }

  bool execute_test()
  {
    print_test_name();

    // Insert a 0 at the beginning of y if necessary
    y.insert(y.begin(), 0);

    // Initialize branch-and-bound solver and statistics
    BBSolverConfig config(0, nullptr);
    config.verbose = verbose;
    BBSolver solver(config);

    // Set y values and update x for each hour
    if (!solver.set_y(y))
    {
      Console::printf(Console::Color::RED, "Error: Failed to update x from y.\n");
      return false;
    }

    double cost = 0.0;
    bool is_feasible = solver.process_node(cost, verbose, config.save_project);
    if (!is_feasible)
    {
      Console::printf(Console::Color::RED, "Error: Process node returned infeasible solution.\n");
      return false;
    }

    return verify_cost(cost, expected_cost, tolerance);
  }
};

class TestCost1 : public TestCostBase
{
public:
  TestCost1() : TestCostBase({1, 2, 1, 2, 1, 1, 1, 1, 0, 0, 2, 2, 2, 2, 2, 1, 2, 1, 0, 0, 0, 2, 1, 0}, 3578.66, 0.01, "test_cost_1")
  {
  }
};

class TestCost2 : public TestCostBase
{
public:
  TestCost2() : TestCostBase({1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1}, 3916.98, 0.01, "test_cost_2")
  {
  }
};

class TestCost3 : public TestCostBase
{
public:
  TestCost3() : TestCostBase({1, 1, 1, 0, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 1, 1, 1, 1}, 3786.74, 0.01, "test_cost_3")
  {
  }
};

class TestTopLevel : public BBTest
{
protected:
public:
  TestTopLevel(int top_level, int top_cut, int expected_top_level)
  {
    this->test_name = "test_top_level";
  }

  bool run() override
  {
    set_up();
    return execute_test();
  }

  bool execute_test()
  {
    Console::printf(Console::Color::BRIGHT_YELLOW, "Running %s...\n", test_name.c_str());
    bool all_tests_passed = true;

    // Define test cases as a vector of tuples
    // Each tuple contains y vector, initial top_level, initial top_cut and expected top_level and expected top_cuts
    std::vector<std::tuple<std::vector<int>, int, int, int>> test_cases = {// Test case 1
                                                                           {{0, 1, 2, 1, 2, 1, 1}, 1, 3, 1},
                                                                           // Test case 2
                                                                           {{0, 3, 3, 1, 2, 2, 3}, 1, 3, 3},
                                                                           // Test case 3
                                                                           {{0, 3, 3, 3, 3, 3, 2}, 1, 3, 6},
                                                                           // Test case 4
                                                                           {{0, 3, 3, 3, 3, 3, 3}, 1, 3, 6},
                                                                           // Test case 5
                                                                           {{0, 0, 0, 2, 1, 0, 0}, 3, 3, 3},
                                                                           // Test case 6
                                                                           {{0, 0, 0, 2, 3, 0, 0}, 3, 2, 5}};

    // Iterate through each test case
    for (size_t i = 0; i < test_cases.size(); ++i)
    {
      auto [y, top_level, top_cut, expected_top_level] = test_cases[i];

      // Initialize BBsolver with provided parameters
      BBSolverConfig config(0, nullptr);
      config.verbose = verbose;
      config.h_max = 6;
      BBSolver solver(config);
      solver.h_min = top_level;
      solver.h_cut = top_cut;

      // Set the y vector
      bool set_y_result = solver.set_y(y);
      if (!set_y_result)
      {
        Console::printf(Console::Color::RED, "Test Case %d: Failed to set y vector.\n", i + 1);
        all_tests_passed = false;
        continue;
      }

      // Call top_level_free
      int result = solver.get_free_level();

      // Verify the result
      if (result == expected_top_level)
      {
        if (verbose) Console::printf(Console::Color::GREEN, "  Test Case %d: Passed.\n", i + 1);
      }
      else
      {
        if (verbose)
        {
          Console::printf(Console::Color::RED, "  Test Case %d: Failed.\n", i + 1);
          Console::printf(Console::Color::RED, "    Expected top level %d, got %d.\n", expected_top_level, result);
        }
        all_tests_passed = false;
      }
    }

    if (all_tests_passed)
    {
      Console::printf(Console::Color::GREEN, "All cases passed.\n");
    }
    else
    {
      Console::printf(Console::Color::RED, "Some cases failed.\n");
    }

    return all_tests_passed;
  }
};

class TestMPI : public BBTest
{
protected:
  std::vector<int> y = {1, 2, 1, 2, 1, 1, 1, 1, 0, 0, 2, 2, 2, 2, 2, 1, 2, 1, 0, 0, 0, 2, 1, 0};
  double expected_cost = 3578.66;
  double tolerance = 0.01;

public:
  TestMPI()
  {
    this->test_name = "test_mpi";
    // Insert a 0 at the beginning of y
    y.insert(y.begin(), 0);
  }

  bool run() override
  {
    set_up();
    return execute_test();
  }

  bool execute_test()
  {
    print_test_name();

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    BBSolverConfig config(0, nullptr);
    config.verbose = verbose;

    BBSolver solver(config);

    if (verbose) solver.show();

    if (!solver.set_y(y))
    {
      Console::printf(Console::Color::RED, "TestMPI: Failed to update x from y.\n");
      return false;
    }
    solver.show_xy(verbose);

    bool all_tests_passed = true;
    for (int i = 0; i < 128; ++i)
    {
      double cost = 0.0;
      bool is_feasible = solver.process_node(cost, verbose, config.save_project);
      if (!is_feasible)
      {
        Console::printf(Console::Color::RED, "TestMPI[rank=%d]: Process node returned infeasible solution.\n", rank);
        all_tests_passed = false;
        break;
      }

      // Check if cost is within the expected range
      if (std::abs(cost - expected_cost) > tolerance)
      {
        all_tests_passed = false;
        break;
      }
    }

    if (all_tests_passed)
    {
      Console::printf(Console::Color::GREEN, "TestMPI[rank=%d]: all tests passed.\n", rank);
    }
    else
    {
      Console::printf(Console::Color::RED, "TestMPI[rank=%d]: failed.\n", rank);
    }

    return all_tests_passed;
  }
};

class TestSplit : public BBTest
{
protected:
  int rank;
  int size;
  int niters;

public:
  TestSplit()
  {
    this->test_name = "test_split";
  }

  bool run() override
  {
    set_up();
    return execute_test();
  }

  bool execute_test()
  {
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    print_test_name();

    // Open output file for each rank
    Console::open(rank, true, verbose);

    // Initialize solver
    BBSolverConfig config(0, nullptr);
    config.max_actuations = 1;
    config.h_max = 3;
    BBSolver solver(config);

    // Initialize iteration variables
    niters = 0;
    int done_loc = (rank != 0); // Only rank 0 starts
    int done_all = false;

    // Start timing
    auto tic = std::chrono::high_resolution_clock::now();

    // Main loop
    while (!done_all)
    {
      mock_iteration(solver, done_loc, verbose);
      sync(solver, done_loc, done_all, verbose);
    }

    // Stop timing
    auto toc = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(toc - tic);
    Console::printf(Console::Color::CYAN, "Rank[%d]: niter %d after %ld ms (final)\n", rank, niters, duration.count());

    // Sum niters across all ranks
    int total_niters = 0;
    MPI_Reduce(&niters, &total_niters, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0)
    {
      bool sum_correct = (total_niters == 48);
      if (sum_correct)
      {
        Console::printf(Console::Color::GREEN, "Total iterations across all ranks: %d (correct)\n", total_niters);
      }
      else
      {
        Console::printf(Console::Color::RED, "Total iterations across all ranks: %d (expected 48)\n", total_niters);
      }
      return sum_correct;
    }

    // Close output file
    Console::close();

    return true;
  }

private:
  void mock_iteration(BBSolver &solver, int &done_loc, bool verbose)
  {
    // If the current rank is not done, attempt to update the solver
    if (done_loc)
    {
      // Once done_loc is true, sleep for 1 second to allow other ranks to catch up
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
      return;
    }

    // Attempt to update the solver
    done_loc = !solver.update_y();
    // Go next, the work is done
    if (done_loc)
    {
      if (verbose) Console::printf(Console::Color::CYAN, "Rank[%d]: done_loc %d\n", rank, done_loc);
      return;
    }
    niters++;

    solver.update_x(verbose);
    if (!solver.is_feasible)
    {
      if (verbose) Console::printf(Console::Color::RED, "Rank[%d]: update_x is infeasible.\n", rank);
      solver.add_prune(PruneReason::ACTUATIONS);
      return;
    }
    solver.show_xy(verbose);

    // Sleep for 1 millisecond to simulate work and prevent tight looping
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    // Check feasibility
    solver.is_feasible = solver.is_feasible ? check_feasibility(size, rank, solver, niters, verbose) : false;
    if (!solver.is_feasible)
    {
      // Just for testing, we prune on pressures
      solver.add_prune(PruneReason::PRESSURES);
      return;
    }

    // Update the solver's feasibility flag
    if (solver.is_feasible) solver.add_feasible();
  }

  void sync(BBSolver &solver, int &done_loc, int &done_all, bool verbose)
  {
    const int sync_period = 1;
    const int free_level_max = 1;

    // Periodically check if any rank is done
    if (niters % sync_period != 0 && !done_loc) return;

    std::vector<int> done(size, 0);
    std::vector<int> free_level(size, -1);
    int mpi_error;

    // Synchronize done_all across all ranks
    mpi_error = MPI_Allgather(&done_loc, 1, MPI_INT, done.data(), 1, MPI_INT, MPI_COMM_WORLD);
    if (mpi_error != MPI_SUCCESS) MPI_Abort(MPI_COMM_WORLD, mpi_error);

    // Check if all ranks are done, if so, break
    done_all = std::all_of(done.begin(), done.end(), [](int i) { return i == 1; });
    if (done_all) return;

    // Update top_level_free with data from all ranks
    int free_level_loc = solver.get_free_level();
    mpi_error = MPI_Allgather(&free_level_loc, 1, MPI_INT, free_level.data(), 1, MPI_INT, MPI_COMM_WORLD);
    if (mpi_error != MPI_SUCCESS) MPI_Abort(MPI_COMM_WORLD, mpi_error);

    // The work is not done if the task is split
    bool split_done = solver.try_split(done, free_level, free_level_max, verbose);
    if (done_loc) done_loc = !split_done;

    MPI_Barrier(MPI_COMM_WORLD);
  }

  bool check_feasibility(int size, int rank, const BBSolver &solver, int niters, bool verbose)
  {

    // Calculate the sum of the y vector up to the current time period (h)
    if (solver.h < 0 || solver.h >= (int)solver.y.size())
    {
      // Handle out-of-range h values gracefully
      Console::printf(Console::Color::RED, "Error: Current time period 'h' is out of range.");
      return false;
    }
    int sum_y = std::accumulate(solver.y.begin(), solver.y.begin() + solver.h + 1, 0);

    // Set is_feasible
    bool is_feasible = sum_y == 0;
    if (sum_y > 0) is_feasible = !(sum_y % 3 == 0);
    if (sum_y > 0 && is_feasible) is_feasible = !(sum_y % 5 == 0);

    if (verbose)
    {
      if (is_feasible)
        Console::printf(Console::Color::GREEN, "Rank[%d]: niters=%d, sum_y=%d is feasible.\n\n", rank, niters, sum_y);
      else
        Console::printf(Console::Color::RED, "Rank[%d]: niters=%d, sum_y=%d is not feasible.\n\n", rank, niters, sum_y);
    }
    return is_feasible;
  }
};

void test_all()
{
  int rank = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  // Instantiate and run all tests
  TestCost1 testCost1;
  TestCost2 testCost2;
  TestCost3 testCost3;
  TestTopLevel testTopLevel(1, 3, 1);
  TestMPI testMPI;
  TestSplit testSplit;

  if (rank == 0)
  {
    testCost1.run();
    testCost2.run();
    testCost3.run();
    testTopLevel.run();
  }
  MPI_Barrier(MPI_COMM_WORLD);

  testMPI.run();
  MPI_Barrier(MPI_COMM_WORLD);

  testSplit.run();
  MPI_Barrier(MPI_COMM_WORLD);

  MPI_Finalize();
  exit(EXIT_SUCCESS);
}