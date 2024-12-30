// src/CLI/BBConstraints.cpp

#include "BBConstraints.h"
#include "BBConfig.h"
#include "Profiler.h"

#include "Elements/pattern.h"

#include <algorithm>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <limits>
#include <mpi.h>
#include <sstream>
#include <vector>

// Constructor
BBConstraints::BBConstraints(const BBConfig &config)
{
  // Initialize nodes and tanks with placeholder IDs
  nodes = {{"55", 0}, {"90", 0}, {"170", 0}};
  tanks = {{"65", 0}, {"165", 0}, {"265", 0}};
  pumps = {{"111", 0}, {"222", 0}, {"333", 0}};
  best_cost = std::numeric_limits<double>::max();

  // Retrieve node and tank IDs from the input file
  get_network_elements_indices(config.inpFile);

  all_finished = false;
  finished = false;
}

// Destructor
BBConstraints::~BBConstraints()
{
  // Clean up resources if necessary
}

void BBConstraints::update_best(double cost, std::vector<int> x, std::vector<int> y)
{
  best_cost = std::min(best_cost, cost);
  best_x = x;
  best_y = y;
}

void BBConstraints::sync_best()
{
  ProfileScope scope("sync_best");

  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  // Step 1: Synchronize the finished flags to determine if all processes have finished
  int local_finished = finished ? 1 : 0;
  int global_finished = 0;
  int err = MPI_Allreduce(&local_finished, &global_finished, 1, MPI_INT, MPI_LAND, MPI_COMM_WORLD);
  if (err != MPI_SUCCESS)
  {
    char error_string[MPI_MAX_ERROR_STRING];
    int length;
    MPI_Error_string(err, error_string, &length);
    fprintf(stderr, "MPI_Allreduce (finished flags) failed: %s\n", error_string);
    MPI_Abort(MPI_COMM_WORLD, err);
  }
  all_finished = (global_finished == 1);

  // Step 2: Synchronize the best solution across all processes
  struct BestSolution
  {
    double cost;
    int rank;
  };

  BestSolution local_best = {best_cost, rank};
  BestSolution global_best;

  // Perform a global reduction to find the minimum cost and the rank that has it
  err = MPI_Allreduce(&local_best, &global_best, 1, MPI_DOUBLE_INT, MPI_MINLOC, MPI_COMM_WORLD);
  if (err != MPI_SUCCESS)
  {
    char error_string[MPI_MAX_ERROR_STRING];
    int length;
    MPI_Error_string(err, error_string, &length);
    fprintf(stderr, "MPI_Allreduce (MINLOC) failed: %s\n", error_string);
    MPI_Abort(MPI_COMM_WORLD, err);
  }

  // Broadcast the best_cost from the process with the minimum cost
  err = MPI_Bcast(&best_cost, 1, MPI_DOUBLE, global_best.rank, MPI_COMM_WORLD);
  if (err != MPI_SUCCESS)
  {
    char error_string[MPI_MAX_ERROR_STRING];
    int length;
    MPI_Error_string(err, error_string, &length);
    fprintf(stderr, "MPI_Bcast (best_cost) failed: %s\n", error_string);
    MPI_Abort(MPI_COMM_WORLD, err);
  }

  // Broadcast the sizes of best_x and best_y
  int best_x_size = 0;
  int best_y_size = 0;
  if (rank == global_best.rank)
  {
    best_x_size = static_cast<int>(best_x.size());
    best_y_size = static_cast<int>(best_y.size());
  }

  err = MPI_Bcast(&best_x_size, 1, MPI_INT, global_best.rank, MPI_COMM_WORLD);
  if (err != MPI_SUCCESS)
  {
    char error_string[MPI_MAX_ERROR_STRING];
    int length;
    MPI_Error_string(err, error_string, &length);
    fprintf(stderr, "MPI_Bcast (best_x_size) failed: %s\n", error_string);
    MPI_Abort(MPI_COMM_WORLD, err);
  }

  err = MPI_Bcast(&best_y_size, 1, MPI_INT, global_best.rank, MPI_COMM_WORLD);
  if (err != MPI_SUCCESS)
  {
    char error_string[MPI_MAX_ERROR_STRING];
    int length;
    MPI_Error_string(err, error_string, &length);
    fprintf(stderr, "MPI_Bcast (best_y_size) failed: %s\n", error_string);
    MPI_Abort(MPI_COMM_WORLD, err);
  }

  // Resize the vectors on non-root processes to receive the data
  if (rank != global_best.rank)
  {
    best_x.resize(best_x_size);
    best_y.resize(best_y_size);
  }

  // Broadcast the best_x vector
  if (best_x_size > 0)
  {
    err = MPI_Bcast(best_x.data(), best_x_size, MPI_INT, global_best.rank, MPI_COMM_WORLD);
    if (err != MPI_SUCCESS)
    {
      char error_string[MPI_MAX_ERROR_STRING];
      int length;
      MPI_Error_string(err, error_string, &length);
      fprintf(stderr, "MPI_Bcast (best_x) failed: %s\n", error_string);
      MPI_Abort(MPI_COMM_WORLD, err);
    }
  }

  // Broadcast the best_y vector
  if (best_y_size > 0)
  {
    err = MPI_Bcast(best_y.data(), best_y_size, MPI_INT, global_best.rank, MPI_COMM_WORLD);
    if (err != MPI_SUCCESS)
    {
      char error_string[MPI_MAX_ERROR_STRING];
      int length;
      MPI_Error_string(err, error_string, &length);
      fprintf(stderr, "MPI_Bcast (best_y) failed: %s\n", error_string);
      MPI_Abort(MPI_COMM_WORLD, err);
    }
  }
}

// Function to display the constraints
void BBConstraints::show() const
{
  // Print horizontal line separator
  Console::hline(Console::Color::BRIGHT_WHITE);

  // Print header
  Console::printf(Console::Color::BRIGHT_WHITE, "BBConstraints\n");

  // Print list of node IDs
  Console::printf(Console::Color::BRIGHT_WHITE, "Nodes: [ ");
  for (const auto &node : nodes)
    Console::printf(Console::Color::BRIGHT_WHITE, "%s ", node.first.c_str());
  Console::printf(Console::Color::BRIGHT_WHITE, "]\n");

  // Print list of tank IDs
  Console::printf(Console::Color::BRIGHT_WHITE, "Tanks: [ ");
  for (const auto &tank : tanks)
    Console::printf(Console::Color::BRIGHT_WHITE, "%s ", tank.first.c_str());
  Console::printf(Console::Color::BRIGHT_WHITE, "]\n");

  // Print list of pump IDs
  Console::printf(Console::Color::BRIGHT_WHITE, "Pumps: [ ");
  for (const auto &pump : pumps)
    Console::printf(Console::Color::BRIGHT_WHITE, "%s ", pump.first.c_str());
  Console::printf(Console::Color::BRIGHT_WHITE, "]\n");
}

void BBConstraints::get_network_elements_indices(std::string inpFile)
{
  Project p;
  CHK(p.load(inpFile.c_str()), "BBConstraints::get_network_elements_indices: Load project");

  Network *nw = p.getNetwork();

  // Find node IDs
  for (auto &node : nodes)
  {
    const std::string &node_name = node.first;
    node.second = nw->indexOf(Element::NODE, node_name);
  }

  // Find tank IDs
  for (auto &tank : tanks)
  {
    const std::string &tank_name = tank.first;
    tank.second = nw->indexOf(Element::NODE, tank_name);
  }

  // Find pump IDs
  for (auto &pump : pumps)
  {
    const std::string &pump_name = pump.first;
    pump.second = nw->indexOf(Element::LINK, pump_name);
  }
}

// Function to display pressure status
void BBConstraints::show_pressures(bool is_feasible, const std::string &node_name, double pressure, double threshold)
{
  if (!is_feasible)
    Console::printf(Console::Color::RED, "  \u274C node[%3s]: %.2f < %.2f\n", node_name.c_str(), pressure, threshold);
  else
    Console::printf(Console::Color::GREEN, "  \u2705 node[%3s]: %.2f >= %.2f\n", node_name.c_str(), pressure, threshold);
}

void BBConstraints::show_best() const
{
  Console::printf(Console::Color::BRIGHT_WHITE, "Best solution: cost=%.2f\n", best_cost);
  Console::printf(Console::Color::BRIGHT_WHITE, "  X: [ ");
  for (const auto &x : best_x)
    Console::printf(Console::Color::BRIGHT_WHITE, "%d ", x);
  Console::printf(Console::Color::BRIGHT_WHITE, "]\n");
  Console::printf(Console::Color::BRIGHT_WHITE, "  Y: [ ");
  for (const auto &y : best_y)
    Console::printf(Console::Color::BRIGHT_WHITE, "%d ", y);
  Console::printf(Console::Color::BRIGHT_WHITE, "]\n");
}

// Function to display tank level status
void BBConstraints::show_levels(bool is_feasible, const std::string &tank_name, double level, double level_min, double level_max)
{
  if (!is_feasible)
    Console::printf(Console::Color::RED, "  \u274C tank[%3s]: %.2f not in [%.2f, %.2f]\n", tank_name.c_str(), level, level_min, level_max);
  else
    Console::printf(Console::Color::GREEN, "  \u2705 tank[%3s]: %.2f in [%.2f, %.2f]\n", tank_name.c_str(), level, level_min, level_max);
}

// Function to display stability status
void BBConstraints::show_stability(bool is_feasible, const std::string &tank_name, double level, double initial_level)
{
  if (!is_feasible)
    Console::printf(Console::Color::RED, "  \u274C tank[%3s]: %.2f < %.2f\n", tank_name.c_str(), level, initial_level);
  else
    Console::printf(Console::Color::GREEN, "  \u2705 tank[%3s]: %.2f >= %.2f\n", tank_name.c_str(), level, initial_level);
}

// Function to check node pressures
bool BBConstraints::check_pressures(Project &p, bool verbose)
{
  if (verbose)
  {
    Console::printf(Console::Color::BRIGHT_WHITE, "\nChecking pressures: [ ");
    for (const auto &node : nodes)
    {
      Console::printf(Console::Color::BRIGHT_CYAN, "%s ", node.first.c_str());
    }
    Console::printf(Console::Color::BRIGHT_WHITE, "]\n");
  }

  std::map<std::string, double> thresholds = {{"55", 42}, {"90", 51}, {"170", 30}};
  bool all_ok = true;

  for (const auto &node : nodes)
  {
    const std::string &node_name = node.first;
    const int &node_index = node.second;
    double pressure;

    // Retrieve node pressure
    CHK(EN_getNodeValue(node_index, EN_PRESSURE, &pressure, &p), "Get node pressure");

    // Check if pressure meets the threshold
    bool is_feasible = pressure >= thresholds[node_name];
    if (!is_feasible)
    {
      all_ok = false;
    }

    // Display pressure status
    if (verbose) show_pressures(is_feasible, node_name, pressure, thresholds[node_name]);
  }

  return all_ok;
}

// Function to check tank levels
bool BBConstraints::check_levels(Project &p, bool verbose)
{
  if (verbose)
  {
    Console::printf(Console::Color::BRIGHT_WHITE, "\nChecking levels: [ ");
    for (const auto &tank : tanks)
      Console::printf(Console::Color::BRIGHT_CYAN, "%s ", tank.first.c_str());
    Console::printf(Console::Color::BRIGHT_WHITE, "]\n");
  }

  const double level_min = 66.53;
  const double level_max = 71.53;
  bool all_ok = true;

  for (const auto &tank : tanks)
  {
    const std::string &tank_name = tank.first;
    const int &tank_index = tank.second;
    double level;

    // Retrieve tank level
    CHK(EN_getNodeValue(tank_index, EN_HEAD, &level, &p), "Get tank level");

    // Check if level is within acceptable range
    bool is_feasible = (level >= level_min) && (level <= level_max);
    if (!is_feasible)
    {
      all_ok = false;
    }

    // Display level status
    if (verbose) show_levels(is_feasible, tank_name, level, level_min, level_max);
  }

  return all_ok;
}

// Function to check tank stability
BBPruneReason BBConstraints::check_stability(Project &p, bool verbose)
{
  if (verbose)
  {
    Console::printf(Console::Color::BRIGHT_WHITE, "\nChecking stability: [ ");
    for (const auto &tank : tanks)
      Console::printf(Console::Color::BRIGHT_CYAN, "%s ", tank.first.c_str());
    Console::printf(Console::Color::BRIGHT_WHITE, "]\n");
  }

  const double initial_level = 66.93;
  bool all_ok = true;

  for (const auto &tank : tanks)
  {
    const std::string &tank_name = tank.first;
    const int &tank_index = tank.second;
    double level;

    // Retrieve tank level
    CHK(EN_getNodeValue(tank_index, EN_HEAD, &level, &p), "Get tank level");

    // Check if level meets the initial stability condition
    bool is_feasible = level >= initial_level;
    if (!is_feasible)
    {
      all_ok = false;
    }

    // Display stability status
    if (verbose) show_stability(is_feasible, tank_name, level, initial_level);
  }

  return all_ok ? BBPruneReason::NONE : BBPruneReason::STABILITY;
}

// Function to check the cost
bool BBConstraints::check_cost(Project &p, double &cost, bool verbose)
{
  cost = calc_cost(p);
  bool is_feasible = cost < best_cost;
  if (verbose)
  {
    Console::printf(Console::Color::BRIGHT_WHITE, "\nChecking cost:\n");
    if (is_feasible)
    {
      if (best_cost > 999999999)
        Console::printf(Console::Color::GREEN, "  \u2705 cost=%.2f < cost_max=inf\n", cost);
      else
        Console::printf(Console::Color::GREEN, "  \u2705 cost=%.2f < cost_max=%.2f\n", cost, best_cost);
    }
    else if (best_cost > 999999999)
      Console::printf(Console::Color::RED, "  \u274C cost=%.2f >= cost_max=inf\n", cost);
    else
      Console::printf(Console::Color::RED, "  \u274C cost=%.2f >= cost_max=%.2f\n", cost, best_cost);
  }
  return is_feasible;
}

// Function to calculate the total cost of pump operations
double BBConstraints::calc_cost(Project &p) const
{
  Network *nw = p.getNetwork();
  double cost = 0.0;
  for (const auto &pump : pumps)
  {
    Pump *pump_link = (Pump *)nw->link(pump.second);
    cost += pump_link->pumpEnergy.adjustedTotalCost;
  }
  return cost;
}

// Function to update pump speed patterns
void BBConstraints::update_pumps(Project &p, const int h, const std::vector<int> &x, bool verbose)
{
  // Update pump speed patterns based on vector x
  int j = 0;
  const size_t num_pumps = get_num_pumps();
  for (int i = 1; i <= h; i++)
  {
    const int *xi = &x[num_pumps * i];
    j = 0; // Reset index of the pump pattern
    for (auto &pump : pumps)
    {
      const auto &pump_name = pump.first;
      const auto &pump_index = pump.second;
      Pump *pump_link = (Pump *)p.getNetwork()->link(pump_index);
      FixedPattern *pattern = dynamic_cast<FixedPattern *>(pump_link->speedPattern);
      if (!pattern)
      {
        Console::printf(Console::Color::RED, "  Error: Pump %s does not have a FixedPattern speed pattern.\n", pump_name.c_str());
        continue;
      }

      // Retrieve new speed factor
      double factor_new = static_cast<double>(xi[j++]);
      // Retrieve old speed factor
      const int factor_id = i - 1; // pattern index is 0-based
      // Update speed factor
      pattern->setFactor(factor_id, factor_new);
    }
  }
}

BBPruneReason BBConstraints::check_feasibility(Project &p, const int h, double &cost, bool verbose)
{
  ProfileScope scope("check_feasibility");

  if (!check_cost(p, cost, verbose)) return BBPruneReason::COST;
  if (!check_pressures(p, verbose)) return BBPruneReason::PRESSURES;
  if (!check_levels(p, verbose)) return BBPruneReason::LEVELS;
  return BBPruneReason::NONE;
}

void BBConstraints::to_json(char *fn) const
{
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  if (rank == 0)
  {
    Console::printf(Console::Color::BRIGHT_GREEN, "💾 Writing best solution to file: %s\n", fn);
  }

  nlohmann::json j;
  j["best_cost"] = best_cost;
  j["best_x"] = best_x;
  j["best_y"] = best_y;
  std::ofstream f(fn);
  f << j.dump(2);
}
