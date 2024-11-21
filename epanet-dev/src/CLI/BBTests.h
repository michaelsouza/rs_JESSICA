// src/BBTests.h

#pragma once
#include "Core/network.h"
#include "Core/project.h"
#include "Elements/pump.h"
#include "epanet3.h"

#include "BBConstraints.h"
#include "BBSolver.h"
#include "ColorStream.h" // Include ColorStream header
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
bool run_cost_test(std::vector<int> &y, double expected_cost, double tolerance, const std::string &test_name,
                   const int max_actuations, bool verbose, bool save_project)
{
  // Default values
  const char *inpFile = "/home/michael/github/rs_JESSICA/networks/any-town.inp";
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
    ColorStream::printf(ColorStream::Color::RED, "Error: Input file %s does not exist.", inpFile);
    return false;
  }

  // Initialize branch-and-bound solver and statistics
  BBSolver solver(inpFile, h_max, max_actuations);

  // Set y values and update x for each hour
  if (!solver.set_y(y))
  {
    ColorStream::printf(ColorStream::Color::RED, "Error: Failed to update x from y.");
    return false;
  }
  solver.show_xy(verbose);

  double cost = 0.0;
  bool is_feasible = solver.process_node(cost, verbose, save_project);
  if (!is_feasible)
  {
    ColorStream::printf(ColorStream::Color::RED, "Error: Process node returned infeasible solution.");
    return false;
  }

  // Check if cost is within the expected range
  if (std::abs(cost - expected_cost) > tolerance)
  {
    ColorStream::printf(ColorStream::Color::RED, "Test Failed (%s): cost (%.2f) is not within %.2f of expected %.2f.",
                        test_name.c_str(), cost, tolerance, expected_cost);
    return false;
  }

  // If the check passes
  ColorStream::printf(ColorStream::Color::GREEN, "Test Passed (%s): cost (%.2f) is within %.2f of expected %.2f.\n",
                      test_name.c_str(), cost, tolerance, expected_cost);

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
  ColorStream::printf(ColorStream::Color::BRIGHT_YELLOW, "Running test_top_level_free...\n");
  bool all_tests_passed = true;

  const char *inpFile = "/home/michael/github/rs_JESSICA/networks/any-town.inp";

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
      ColorStream::printf(ColorStream::Color::RED, "Test Case %d: Failed to set y vector.", i + 1);
      all_tests_passed = false;
      continue;
    }

    // Call top_level_free
    int result = solver.get_free_level();

    // Verify the result
    if (result == expected_top_level_free)
    {
      if (verbose) ColorStream::printf(ColorStream::Color::GREEN, "  Test Case %d: Passed.\n", i + 1);
    }
    else
    {
      if (verbose)
        ColorStream::printf(ColorStream::Color::RED, "  Test Case %d: Failed. Expected %d, got %d.\n", i + 1,
                            expected_top_level_free, result);
      all_tests_passed = false;
    }
  }

  if (all_tests_passed)
  {
    ColorStream::printf(ColorStream::Color::GREEN, "All cases passed.\n");
  }
  else
  {
    ColorStream::printf(ColorStream::Color::RED, "Some cases failed.\n");
  }

  return all_tests_passed;
}

bool test_mpi(bool verbose = false)
{
  std::string test_name = "test_mpi";

  // Default values
  const char *inpFile = "/home/michael/github/rs_JESSICA/networks/any-town.inp";
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
    ColorStream::printf(ColorStream::Color::RED, "test_mpi: Input file %s does not exist.", inpFile);
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
    ColorStream::printf(ColorStream::Color::RED, "test_mpi: Failed to update x from y.");
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
      ColorStream::printf(ColorStream::Color::RED, "test_mpi: Process node returned infeasible solution.");
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
    ColorStream::printf(ColorStream::Color::GREEN, "test_mpi[rank=%d]: all tests passed.\n", rank);
  }
  else
  {
    ColorStream::printf(ColorStream::Color::RED, "test_mpi[rank=%d]: failed.\n", rank);
  }

  return all_tests_passed;
}

bool test_split()
{
  // Define the parameters
  int h_max = 3;
  int max_actuations = 3;

  // in order to split top_level_free should be lower than top_level_max
  int top_level_max = 4;
  // sync period, only sync if niters is a multiple of sync_period
  int sync_period = 1;

  // Initialize the solver
  const char *inpFile = "/home/michael/github/rs_JESSICA/networks/any-town.inp";
  BBSolver solver(inpFile, h_max, max_actuations);

  int rank, size;
  MPI_Init(NULL, NULL);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  // Using int instead of bool because the memory layout is MPI friendly
  int done_loc = rank != 0; // Only rank 0 starts
  int done_all = false;
  std::vector<int> done(size, 0);
  int is_feasible = 1; // Initialize as true
  int mpi_error;
  std::vector<int> recv_buffer(6 + solver.y.size() + solver.x.size());

  int niters = 0;
  auto tic = std::chrono::high_resolution_clock::now();
  std::vector<int> top_level_free(size, -1);

  // Define the lambda function to check feasibility
  auto check_feasibility = [&](const BBSolver &cnt) -> bool
  {
    // Calculate the sum of the y vector up to the current time period (h)
    if (cnt.h < 0 || cnt.h >= (int)cnt.y.size())
    {
      // Handle out-of-range h values gracefully
      ColorStream::printf(ColorStream::Color::RED, "Error: Current time period 'h' is out of range.");
      return false;
    }
    int sum_y = std::accumulate(cnt.y.begin(), cnt.y.begin() + cnt.h + 1, 0);
    // Return false if sum_y is a multiple of 7, true otherwise
    return (sum_y % 7 != 0);
  };

  while (!done_all)
  {
    // If the current rank is not done, attempt to update the solver
    if (!done_loc)
    {
      // Attempt to update the solver
      niters++;
      done_loc = !solver.update_y();

      // Go next, the work is done
      if (done_loc) continue;

      // Sleep for 1 millisecond to simulate work and prevent tight looping
      std::this_thread::sleep_for(std::chrono::milliseconds(1));

      // Use the lambda to determine feasibility based on the updated solver
      is_feasible = check_feasibility(solver);
      if (!is_feasible) solver.prune(PruneReason::ACTUATIONS);
    }
    else
    {
      // Once done_loc is true, sleep for 1 second to allow other ranks to catch up
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // Periodically check if any rank is done
    if (niters % sync_period == 0 || done_loc)
    {
      auto toc = std::chrono::high_resolution_clock::now();
      auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(toc - tic);
      printf("Rank[%d]: niter %d after %ld ms\n", rank, niters, duration.count());

      // Synchronize done_all across all ranks
      mpi_error = MPI_Allgather(&done_loc, 1, MPI_INT, done.data(), 1, MPI_INT, MPI_COMM_WORLD);
      if (mpi_error != MPI_SUCCESS)
      {
        printf("Rank[%d]: MPI_Allreduce failed with error code %d.\n", rank, mpi_error);
        MPI_Abort(MPI_COMM_WORLD, mpi_error);
      }

      // Check if all ranks are done, if so, break
      done_all = std::all_of(done.begin(), done.end(), [](int i) { return i == 1; });
      if (done_all)
      {
        break;
      }

      // Update top_level_free with data from all ranks
      int top_level_free_loc = solver.get_free_level();
      mpi_error = MPI_Allgather(&top_level_free_loc, 1, MPI_INT, top_level_free.data(), 1, MPI_INT, MPI_COMM_WORLD);
      if (mpi_error != MPI_SUCCESS)
      {
        printf("Rank[%d]: MPI_Allgather failed with error code %d.\n", rank, mpi_error);
        MPI_Abort(MPI_COMM_WORLD, mpi_error);
      }

      printf("Rank[%d]: top_level_free: [ ", rank);
      for (int i = 0; i < size; i++)
      {
        printf("%d ", top_level_free[i]);
      }
      printf("]\n");
      printf("Rank[%d]: done_loc %d\n", rank, done_loc);
      printf("Rank[%d]: done_all %d\n", rank, done_all);
      MPI_Barrier(MPI_COMM_WORLD);

      for (int id_avail = 0, count_avail = 0; id_avail < size; ++id_avail)
      {
        // There is no need to split if the rank is not done
        if (!done[id_avail]) continue;

        // Count the number of available ranks
        ++count_avail;

        for (int id_split = 0, count_split = 0; id_split < size; ++id_split)
        {
          // There is no need to split if the rank is already done
          if (done[id_split]) continue;

          // There is no need to split if the top_level_free is greater than the max
          if (top_level_free[id_split] > top_level_max) continue;

          // Count the number of split ranks
          ++count_split;
          // There is no need of action if the count of available ranks is not equal to the count of split ranks
          if (count_avail != count_split) continue;

          // Receive a message from rank id_split
          if (id_avail == rank)
          {
            ColorStream::printf(ColorStream::Color::BRIGHT_MAGENTA, "Rank[%d]: Receiving from rank %d", rank, id_split);
            mpi_error = MPI_Recv(recv_buffer.data(), recv_buffer.size(), MPI_INT, id_split, 0, MPI_COMM_WORLD,
                                 MPI_STATUS_IGNORE);
            if (mpi_error != MPI_SUCCESS)
            {
              printf("Rank[%d]: MPI_Recv failed with error code %d.\n", rank, mpi_error);
              MPI_Abort(MPI_COMM_WORLD, mpi_error);
            }
            // Set the current solver
            done_loc = 0;
            solver.read_buffer(recv_buffer);
            solver.show();
            is_feasible = 1;
          }

          // Send a message to rank id_avail
          if (id_split == rank)
          {
            ColorStream::printf(ColorStream::Color::BRIGHT_MAGENTA, "Rank[%d]: Sending to rank %d", rank, id_avail);
            solver.write_buffer(recv_buffer);
            mpi_error = MPI_Send(recv_buffer.data(), recv_buffer.size(), MPI_INT, id_avail, 0, MPI_COMM_WORLD);
            if (mpi_error != MPI_SUCCESS)
            {
              printf("Rank[%d]: MPI_Send failed with error code %d.\n", rank, mpi_error);
              MPI_Abort(MPI_COMM_WORLD, mpi_error);
            }
          }
          break;
        }
      }
    }
  }

  // Add MPI barrier to synchronize all processes
  MPI_Barrier(MPI_COMM_WORLD);

  // Final log after exiting the loop
  auto toc = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(toc - tic);
  printf("Rank[%d]: niter %d after %ld ms (final)\n", rank, niters, duration.count());

  MPI_Finalize();

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
  if (rank == 0)
  {
    test_cost_1(false);
    test_cost_2(false);
    test_cost_3(false);
    test_top_level_free(false);
  }
  MPI_Barrier(MPI_COMM_WORLD);

  test_mpi(false);
  // test_split();

  MPI_Finalize();
}
