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
#include <numeric>
#include <string>
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

void test_all()
{
  // test_cost_1();
  // test_cost_2();
  // test_cost_3();
  test_cost_4();
}
