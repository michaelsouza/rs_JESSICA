// src/BBTests.h

#pragma once
#include "Core/network.h"
#include "Core/project.h"
#include "Elements/pump.h"
#include "epanet3.h"

#include "BBConstraints.h"
#include "BBSolver.h"
#include "Console.h"
#include "Utils.h"

#include <algorithm>
#include <cassert> // For assert
#include <chrono>
#include <cmath>   // For std::abs
#include <cstdlib> // For std::atoi
#include <cstring>
#include <fstream> // Required for checking file existence
#include <iostream>
#include <limits>
#include <map>
#include <mpi.h>
#include <numeric>
#include <string>
#include <thread> // Required for sleep
#include <vector>

// Generic test function to avoid code duplication
bool run_cost_test(std::vector<int> &y, double expected_cost, double tolerance, const std::string &test_name, const int max_actuations, bool verbose,
                   bool save_project)
{
  // Default values
  const char *inpFile = "/home/michael/gitrepos/rs_JESSICA/networks/any-town.inp";
  int h_max = 24;

  if ((int)y.size() == h_max)
  {
    // Insert a 0 at the beginning of y, because the
    // solver expects the first element to be 0
    y.insert(y.begin(), 0);
  }

  // Check if the inpFile exists
  std::ifstream fileCheck(inpFile);
  if (!fileCheck)
  {
    Console::printf(Console::Color::RED, "Error: Input file %s does not exist.", inpFile);
    return false;
  }

  // Initialize branch-and-bound solver and statistics
  BBSolver solver(inpFile, h_max, max_actuations);

  // Set y values and update x for each hour
  if (!solver.set_y(y))
  {
    Console::printf(Console::Color::RED, "Error: Failed to update x from y.");
    return false;
  }
  solver.show_xy(verbose);

  double cost = 0.0;
  bool is_feasible = solver.process_node(cost, verbose, save_project);
  if (!is_feasible)
  {
    Console::printf(Console::Color::RED, "Error: Process node returned infeasible solution.");
    return false;
  }

  // Check if cost is within the expected range
  if (std::abs(cost - expected_cost) > tolerance)
  {
    Console::printf(Console::Color::RED, "Test Failed (%s): cost (%.2f) is not within %.2f of expected %.2f.", test_name.c_str(), cost, tolerance,
                    expected_cost);
    return false;
  }

  // If the check passes
  Console::printf(Console::Color::GREEN, "Test Passed (%s): cost (%.2f) is within %.2f of expected %.2f.\n", test_name.c_str(), cost, tolerance,
                  expected_cost);

  return true;
}

bool test_cost_1(bool verbose = false)
{
  std::vector<int> y = {1, 2, 1, 2, 1, 1, 1, 1, 0, 0, 2, 2, 2, 2, 2, 1, 2, 1, 0, 0, 0, 2, 1, 0};
  double expected_cost_min = 3578.66; // Costa2015
  double tolerance = 0.01;
  std::string test_name = "test_cost_1";
  int max_actuations = 3;
  bool save_project = false;
  return run_cost_test(y, expected_cost_min, tolerance, test_name, max_actuations, verbose, save_project);
}

bool test_cost_2(bool verbose = false)
{
  std::vector<int> y = {1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1};
  double expected_cost_min = 3916.98; // Costa2015
  double tolerance = 0.01;
  std::string test_name = "test_cost_2";
  int max_actuations = 3;
  bool save_project = false;
  return run_cost_test(y, expected_cost_min, tolerance, test_name, max_actuations, verbose, save_project);
}

bool test_cost_3(bool verbose = false)
{
  std::vector<int> y = {1, 1, 1, 0, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 1, 1, 1, 1};
  double expected_cost_min = 3786.74;
  double tolerance = 0.01;
  std::string test_name = "test_cost_3";
  int max_actuations = 1;
  bool save_project = false;

  return run_cost_test(y, expected_cost_min, tolerance, test_name, max_actuations, verbose, save_project);
}

bool test_top_level_free(bool verbose = false)
{
  Console::printf(Console::Color::BRIGHT_YELLOW, "Running test_top_level_free...\n");
  bool all_tests_passed = true;

  const char *inpFile = "/home/michael/gitrepos/rs_JESSICA/networks/any-town.inp";

  // Define test cases as a vector of tuples
  // Each tuple contains:
  // - y vector
  // - h_max
  // - max_actuations
  // - num_pumps
  // - top_level
  // - top_cut
  // - expected top_level_free result
  int top_cut = 3;
  int top_level = 1;
  int expected_top_level_free = 1;
  int max_actuations = 3;
  int h_max = 6;

  std::vector<std::tuple<std::vector<int>, int, int, int, int, int>> test_cases;

  // Test Case 1:
  test_cases.push_back({{0, 1, 2, 1, 2, 1, 1}, h_max, max_actuations, top_level, top_cut, expected_top_level_free});

  // Test Case 2:
  expected_top_level_free = 3;
  test_cases.push_back({{0, 3, 3, 1, 2, 2, 3}, h_max, max_actuations, top_level, top_cut, expected_top_level_free});

  // Test Case 3:
  expected_top_level_free = 6;
  test_cases.push_back({{0, 3, 3, 3, 3, 3, 2}, h_max, max_actuations, top_level, top_cut, expected_top_level_free});

  // Test Case 4:
  expected_top_level_free = 6;
  test_cases.push_back({{0, 3, 3, 3, 3, 3, 3}, h_max, max_actuations, top_level, top_cut, expected_top_level_free});

  // Test Case 5:
  top_level = 3;
  expected_top_level_free = 3;
  test_cases.push_back({{0, 0, 0, 2, 1, 0, 0}, h_max, max_actuations, top_level, top_cut, expected_top_level_free});

  // Test Case 6:
  top_level = 3;
  top_cut = 2;
  expected_top_level_free = 4;
  test_cases.push_back({{0, 0, 0, 2, 1, 0, 0}, h_max, max_actuations, top_level, top_cut, expected_top_level_free});

  // Test Case 7:
  top_level = 3;
  top_cut = 2;
  expected_top_level_free = 5;
  test_cases.push_back({{0, 0, 0, 2, 3, 0, 0}, h_max, max_actuations, top_level, top_cut, expected_top_level_free});

  // Iterate through each test case
  for (size_t i = 0; i < test_cases.size(); ++i)
  {
    auto [y, h_max, max_actuations, top_level, top_cut, expected_top_level_free] = test_cases[i];

    // Initialize BBsolver with provided parameters
    BBSolver solver(inpFile, h_max, max_actuations);
    solver.top_level = top_level;
    solver.top_cut = top_cut;

    // Set the y vector
    bool set_y_result = solver.set_y(y);
    if (!set_y_result)
    {
      Console::printf(Console::Color::RED, "Test Case %d: Failed to set y vector.", i + 1);
      all_tests_passed = false;
      continue;
    }

    // Call top_level_free
    int result = solver.get_free_level();

    // Verify the result
    if (result == expected_top_level_free)
    {
      if (verbose) Console::printf(Console::Color::GREEN, "  Test Case %d: Passed.\n", i + 1);
    }
    else
    {
      if (verbose) Console::printf(Console::Color::RED, "  Test Case %d: Failed. Expected %d, got %d.\n", i + 1, expected_top_level_free, result);
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

bool test_mpi(bool verbose = false)
{
  std::string test_name = "test_mpi";

  // Default values
  const char *inpFile = "/home/michael/gitrepos/rs_JESSICA/networks/any-town.inp";
  int h_max = 24;
  int max_actuations = 3;
  bool save_project = false;
  double tolerance = 0.01;
  double expected_cost = 3578.66;

  std::vector<int> y = {1, 2, 1, 2, 1, 1, 1, 1, 0, 0, 2, 2, 2, 2, 2, 1, 2, 1, 0, 0, 0, 2, 1, 0};
  // Insert a 0 at the beginning of y, because the
  // solver expects the first element to be 0
  y.insert(y.begin(), 0);

  // Check if the inpFile exists
  std::ifstream fileCheck(inpFile);
  if (!fileCheck)
  {
    Console::printf(Console::Color::RED, "test_mpi: Input file %s does not exist.", inpFile);
    return false;
  }

  // Get MPI rank
  int rank = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  // Initialize branch-and-bound solver and statistics
  BBSolver solver(inpFile, h_max, max_actuations);

  if (verbose) solver.show();

  // Set y values and update x for each hour
  if (!solver.set_y(y))
  {
    Console::printf(Console::Color::RED, "test_mpi: Failed to update x from y.");
    return false;
  }
  solver.show_xy(verbose);

  bool all_tests_passed = true;
  for (int i = 0; i < 128; ++i)
  {
    double cost = 0.0;
    bool is_feasible = solver.process_node(cost, verbose, save_project);
    if (!is_feasible)
    {
      Console::printf(Console::Color::RED, "test_mpi: Process node returned infeasible solution.");
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
    Console::printf(Console::Color::GREEN, "test_mpi[rank=%d]: all tests passed.\n", rank);
  }
  else
  {
    Console::printf(Console::Color::RED, "test_mpi[rank=%d]: failed.\n", rank);
  }

  return all_tests_passed;
}

bool test_split_check_feasibility(int size, int rank, const BBSolver &solver, int niters, bool verbose)
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
      Console::printf(Console::Color::GREEN, "Rank[%d]: niters=%d, sum_y=%d is feasible.\n", rank, niters, sum_y);
    else
      Console::printf(Console::Color::RED, "Rank[%d]: niters=%d, sum_y=%d is not feasible.\n", rank, niters, sum_y);
  }
  return is_feasible;
}

void test_split_mock_iteration(int size, int rank, BBSolver &solver, int &niters, int &done_loc, bool verbose)
{
  // If the current rank is not done, attempt to update the solver
  if (!done_loc)
  {
    // Attempt to update the solver
    done_loc = !solver.update_y();
    // Go next, the work is done
    if (done_loc)
    {
      if (verbose) Console::printf(Console::Color::CYAN, "Rank[%d]: done_loc %d\n", rank, done_loc);
      return;
    }
    niters++;

    bool is_feasible = solver.update_x(verbose);
    if (!is_feasible)
    {
      if (verbose) Console::printf(Console::Color::RED, "Rank[%d]: update_x is infeasible.\n", rank);
      solver.prune(PruneReason::ACTUATIONS);
      return;
    }
    solver.show_xy(verbose);

    // Sleep for 1 millisecond to simulate work and prevent tight looping
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    // Check feasibility
    is_feasible = is_feasible ? test_split_check_feasibility(size, rank, solver, niters, verbose) : false;
    if (!is_feasible)
    {
      solver.prune(PruneReason::SPLIT);
      return;
    }

    // Update the solver's feasibility flag
    if (is_feasible)
    {
      solver.set_feasible();
    }
  }
  else
  {
    // Once done_loc is true, sleep for 1 second to allow other ranks to catch up
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
}

void test_split_sync(int size, int rank, BBSolver &solver, int &niters, int &done_loc, int &done_all, bool verbose)
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

  solver.split(done, free_level, free_level_max, verbose);

  MPI_Barrier(MPI_COMM_WORLD);
}

bool test_split(bool verbose = false)
{
  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  // Open output file for each rank
  Console::open(rank, true, verbose);

  // Define the parameters
  int h_max = 3;
  int max_actuations = 1;

  // Initialize the solver
  const char *inpFile = "/home/michael/gitrepos/rs_JESSICA/networks/any-town.inp";
  BBSolver solver(inpFile, h_max, max_actuations);

  // Using int instead of bool because the memory layout is MPI friendly
  int done_loc = (rank != 0); // Only rank 0 starts
  int done_all = false;

  // Initialize as true
  int niters = 0;
  auto tic = std::chrono::high_resolution_clock::now();
  while (!done_all)
  {
    test_split_mock_iteration(size, rank, solver, niters, done_loc, verbose);
    test_split_sync(size, rank, solver, niters, done_loc, done_all, verbose);
  }
  // Final log after exiting the loop
  auto toc = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(toc - tic);
  Console::printf(Console::Color::CYAN, "Rank[%d]: niter %d after %ld ms (final)\n", rank, niters, duration.count());

  // Close the output file
  Console::close();
  return true;
}

void test_all()
{
  int rank = 0;
  int mpi_error = MPI_Init(nullptr, nullptr);
  if (mpi_error != MPI_SUCCESS)
  {
    printf("MPI_Init failed with error code %d\n", mpi_error);
    return;
  }
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  // Single rank
  // if (rank == 0)
  // {
  //   test_cost_1(false);
  //   test_cost_2(false);
  //   test_cost_3(false);
  //   test_top_level_free(false);
  // }
  // MPI_Barrier(MPI_COMM_WORLD);

  // test_mpi(false);
  // MPI_Barrier(MPI_COMM_WORLD);

  test_split(true);
  MPI_Barrier(MPI_COMM_WORLD);

  MPI_Finalize();
}