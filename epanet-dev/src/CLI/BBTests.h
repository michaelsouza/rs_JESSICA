// src/BBTests.h

#pragma once
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
#include <thread>
#include <vector>

// Generic test function to avoid code duplication
bool run_cost_test(std::vector<int> &y, double expected_cost_min, double tolerance, const std::string &test_name,
                   const int max_actuations, bool verbose, bool save_project)
{
  // Default values
  const char *inpFile = "/home/michael/gitrepos/rs_JESSICA/networks/any-town.inp";
  int h_max = 24;

  if ((int)y.size() == h_max)
  {
    // Insert a 0 at the beginning of y, because the
    // counter expects the first element to be 0
    y.insert(y.begin(), 0);
  }

  // Check if the inpFile exists
  std::ifstream fileCheck(inpFile);
  if (!fileCheck)
  {
    ColorStream::println("Error: Input file " + std::string(inpFile) + " does not exist.", ColorStream::Color::RED);
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

  // Set y values and update x for each hour
  if (!counter.set_y(y))
  {
    ColorStream::println("Error: Failed to update x from y.", ColorStream::Color::RED);
    return false;
  }
  counter.show_xy(verbose);

  double cost = 0.0;
  bool is_feasible = process_node(inpFile, counter, stats, nodes, tanks, pump_names, cost, verbose, save_project);
  if (!is_feasible)
  {
    ColorStream::println("Error: Process node returned infeasible solution.", ColorStream::Color::RED);
    return false;
  }

  // Output the computed cost
  printf("cost: %.2f\n", cost);

  // Check if stats.cost_min is within the expected range
  if (std::abs(stats.cost_min - expected_cost_min) > tolerance)
  {
    ColorStream::println("Test Failed (" + test_name + "): stats.cost_min (" + std::to_string(stats.cost_min) +
                             ") is not within " + std::to_string(tolerance) + " of expected " +
                             std::to_string(expected_cost_min) + ".",
                         ColorStream::Color::RED);
    return false;
  }

  // If the check passes
  ColorStream::println("Test Passed (" + test_name + "): stats.cost_min (" + std::to_string(stats.cost_min) +
                           ") is within " + std::to_string(tolerance) + " of expected " +
                           std::to_string(expected_cost_min) + ".",
                       ColorStream::Color::GREEN);

  return true;
}

bool test_cost_1()
{
  std::vector<int> y = {1, 2, 1, 2, 1, 1, 1, 1, 0, 0, 2, 2, 2, 2, 2, 1, 2, 1, 0, 0, 0, 2, 1, 0};
  double expected_cost_min = 3578.66; // Costa2015
  double tolerance = 0.01;
  std::string test_name = "test_cost";
  int max_actuations = 3;
  bool verbose = true;
  bool save_project = false;
  return run_cost_test(y, expected_cost_min, tolerance, test_name, max_actuations, verbose, save_project);
}

bool test_cost_2()
{
  std::vector<int> y = {1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1};
  double expected_cost_min = 3916.98; // Costa2015
  double tolerance = 0.01;
  std::string test_name = "test_cost_2";
  int max_actuations = 3;
  bool verbose = false;
  bool save_project = true;
  return run_cost_test(y, expected_cost_min, tolerance, test_name, max_actuations, verbose, save_project);
}

bool test_cost_3()
{
  std::vector<int> y = {1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 1, 1, 1, 0};
  double expected_cost_min = 3578.66;
  double tolerance = 0.01;
  std::string test_name = "test_cost_3";
  int max_actuations = 3;
  bool verbose = true;
  bool save_project = false;
  return run_cost_test(y, expected_cost_min, tolerance, test_name, max_actuations, verbose, save_project);
}

bool test_cost_4()
{
  std::vector<int> y = {1, 1, 1, 0, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 1, 1, 1, 1};
  double expected_cost_min = 3916.98;
  double tolerance = 0.01;
  std::string test_name = "test_cost_4";
  int max_actuations = 1;
  bool verbose = true;
  bool save_project = true;

  return run_cost_test(y, expected_cost_min, tolerance, test_name, max_actuations, verbose, save_project);
}

bool test_top_level_free()
{
  std::cout << "Running test_top_level_free..." << std::endl;
  bool all_tests_passed = true;

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
  int num_pumps = 3;
  int h_max = 6;
  int y_max = 3;

  std::vector<std::tuple<std::vector<int>, int, int, int, int, int, int>> test_cases;

  // Test Case 1:
  test_cases.push_back(
      {{0, 1, 2, 1, 2, 1, 1}, h_max, max_actuations, num_pumps, top_level, top_cut, expected_top_level_free});

  // Test Case 2:
  expected_top_level_free = 3;
  test_cases.push_back(
      {{0, 3, 3, 1, 2, 2, 3}, h_max, max_actuations, num_pumps, top_level, top_cut, expected_top_level_free});

  // Test Case 3:
  expected_top_level_free = 6;
  test_cases.push_back(
      {{0, 3, 3, 3, 3, 3, 2}, h_max, max_actuations, num_pumps, top_level, top_cut, expected_top_level_free});

  // Test Case 4:
  expected_top_level_free = 6;
  test_cases.push_back(
      {{0, 3, 3, 3, 3, 3, 3}, h_max, max_actuations, num_pumps, top_level, top_cut, expected_top_level_free});

  // Test Case 5:
  top_level = 3;
  expected_top_level_free = 3;
  test_cases.push_back(
      {{0, 0, 0, 2, 1, 0, 0}, h_max, max_actuations, num_pumps, top_level, top_cut, expected_top_level_free});

  // Test Case 6:
  top_level = 3;
  top_cut = 2;
  expected_top_level_free = 4;
  test_cases.push_back(
      {{0, 0, 0, 2, 1, 0, 0}, h_max, max_actuations, num_pumps, top_level, top_cut, expected_top_level_free});

  // Test Case 7:
  top_level = 3;
  top_cut = 2;
  expected_top_level_free = 5;
  test_cases.push_back(
      {{0, 0, 0, 2, 3, 0, 0}, h_max, max_actuations, num_pumps, top_level, top_cut, expected_top_level_free});

  // Iterate through each test case
  for (size_t i = 0; i < test_cases.size(); ++i)
  {
    auto [y, h_max, max_actuations, num_pumps, top_level, top_cut, expected_top_level_free] = test_cases[i];

    // Initialize BBCounter with provided parameters
    BBCounter counter(y_max, h_max, max_actuations, num_pumps);
    counter.top_level = top_level;
    counter.top_cut = top_cut;

    // Set the y vector
    bool set_y_result = counter.set_y(y);
    if (!set_y_result)
    {
      ColorStream::println("Test Case " + std::to_string(i + 1) + ": Failed to set y vector.", ColorStream::Color::RED);
      all_tests_passed = false;
      continue;
    }

    // Call top_level_free
    int result = counter.top_level_free();

    // Verify the result
    if (result == expected_top_level_free)
    {
      ColorStream::println("Test Case " + std::to_string(i + 1) + ": Passed.", ColorStream::Color::GREEN);
    }
    else
    {
      ColorStream::println("Test Case " + std::to_string(i + 1) + ": Failed. Expected " +
                               std::to_string(expected_top_level_free) + ", got " + std::to_string(result) + ".",
                           ColorStream::Color::RED);
      all_tests_passed = false;
    }
  }

  if (all_tests_passed)
  {
    ColorStream::println("All test_top_level_free cases passed.", ColorStream::Color::GREEN);
  }
  else
  {
    ColorStream::println("Some test_top_level_free cases failed.", ColorStream::Color::RED);
  }

  return all_tests_passed;
}

bool test_mpi()
{
  printf("Testing MPI...\n");
  const char *inpFile = "/home/michael/github/rs_JESSICA/networks/any-town.inp";
  int h_max = 24;
  bool verbose = false;
  bool save_project = false;
  int max_actuations = 3;

  // Check if the inpFile exists
  std::ifstream fileCheck(inpFile);
  if (!fileCheck)
  {
    ColorStream::println("Error: Input file " + std::string(inpFile) + " does not exist.", ColorStream::Color::RED);
    return false;
  }

  std::map<std::string, int> nodes = {{"55", 0}, {"90", 0}, {"170", 0}};
  std::map<std::string, int> tanks = {{"65", 0}, {"165", 0}, {"265", 0}};
  std::vector<std::string> pump_names = {"111", "222", "333"};

  get_nodes_and_tanks_ids(inpFile, nodes, tanks, verbose);
  show_nodes_pumps_tanks(nodes, pump_names, tanks, verbose);

  BBCounter counter(nodes.size(), h_max, max_actuations, pump_names.size());
  BBStats stats(h_max, max_actuations);

  std::vector<int> y = {0, 1, 2, 1, 2, 1, 1, 1, 1, 0, 0, 2, 2, 2, 2, 2, 1, 2, 1, 0, 0, 0, 2, 1, 0};
  counter.set_y(y);

  auto start = std::chrono::high_resolution_clock::now();
  int rank, size;
  MPI_Init(NULL, NULL);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  int iterations_per_rank = 256 / size;
  int start_iter = rank * iterations_per_rank;
  int end_iter = (rank == size - 1) ? 256 : (rank + 1) * iterations_per_rank;

  for (int i = start_iter; i < end_iter; i++)
  {
    double cost = 0.0;
    process_node(inpFile, counter, stats, nodes, tanks, pump_names, cost, verbose, save_project);
    // assert cost close to 3578.67
    assert(std::abs(cost - 3578.67) < 0.01);
  }

  printf("Rank %d finished processing iterations %d to %d\n", rank, start_iter, end_iter - 1);

  MPI_Finalize();
  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  printf("Time taken: %ld ms\n", duration.count());

  return true;
}

bool test_split()
{
  int h_max = 6;
  int max_actuations = 3;
  int top_level_max = 2;
  BBCounter counter(3, h_max, max_actuations, 3);

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
  std::vector<int> recv_buffer(6 + counter.y.size() + counter.x.size());

  int niters = 0;
  auto tic = std::chrono::high_resolution_clock::now();
  std::vector<int> top_level_free(size, -1);

  // Define the lambda function to check feasibility
  auto check_feasibility = [&](const BBCounter &cnt) -> bool
  {
    // Calculate the sum of the y vector up to the current time period (h)
    if (cnt.h < 0 || cnt.h >= (int)cnt.y.size())
    {
      // Handle out-of-range h values gracefully
      ColorStream::println("Error: Current time period 'h' is out of range.", ColorStream::Color::RED);
      return false;
    }
    int sum_y = std::accumulate(cnt.y.begin(), cnt.y.begin() + cnt.h + 1, 0);
    // Return false if sum_y is a multiple of 7, true otherwise
    return (sum_y % 7 != 0);
  };

  // Optional: Define a maximum iteration limit to prevent infinite loops
  const int MAX_ITERATIONS = 10000; // Example limit

  while (!done_all)
  {
    if (!done_loc)
    {
      // Attempt to update the counter
      niters++;
      done_loc = !counter.update_y(is_feasible);

      // Sleep for 1 millisecond to simulate work and prevent tight looping
      std::this_thread::sleep_for(std::chrono::milliseconds(1));

      // Use the lambda to determine feasibility based on the updated counter
      is_feasible = check_feasibility(counter);
    }
    else
    {
      // Once done_loc is true, sleep for 1 second to allow other ranks to catch up
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // Periodically check if any rank is done
    if (niters % 1024 == 0 || done_loc)
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
      int top_level_free_loc = counter.top_level_free();
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
            printf("Rank[%d]: Receiving from rank %d\n", rank, id_split);            
            mpi_error = MPI_Recv(recv_buffer.data(), recv_buffer.size(), MPI_INT, id_split, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if (mpi_error != MPI_SUCCESS)
            {
              printf("Rank[%d]: MPI_Recv failed with error code %d.\n", rank, mpi_error);
              MPI_Abort(MPI_COMM_WORLD, mpi_error);
            }
            // Reset the done flag for the rank that sent the message
            done_loc = 0;
            counter.read_buffer(recv_buffer);
            counter.show();
          }

          // Send a message to rank id_avail
          if (id_split == rank)
          {
            printf("Rank[%d]: Sending to rank %d\n", rank, id_avail);
            counter.write_buffer(recv_buffer);
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

  if (niters >= MAX_ITERATIONS)
  {
    printf("Rank[%d]: Reached maximum iterations without completion.\n", rank);
  }

  // Final log after exiting the loop
  auto toc = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(toc - tic);
  printf("Rank[%d]: niter %d after %ld ms (final)\n", rank, niters, duration.count());

  MPI_Finalize();

  return true;
}

void test_all()
{
  // test_cost_1();
  // test_cost_2();
  // test_cost_3();
  // test_cost_4();
  // test_top_level_free();
  // test_mpi();
  test_split();
}
