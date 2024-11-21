// src/CLI/BBConstraints.cpp
#include "BBConstraints.h"
#include "Utils.h" // Assuming Utils.h contains the CHK macro and other utilities

#include "Elements/pattern.h"

#include <algorithm>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>

// Constructor
BBConstraints::BBConstraints(std::string inpFile) : inpFile(inpFile)
{
  // Initialize nodes and tanks with placeholder IDs
  this->nodes = {{"55", 0}, {"90", 0}, {"170", 0}};
  this->tanks = {{"65", 0}, {"165", 0}, {"265", 0}};
  this->pump_names = {"111", "222", "333"};
  this->cost_max = std::numeric_limits<double>::max();

  // Retrieve node and tank IDs from the input file
  get_nodes_and_tanks_ids(inpFile);
}

// Destructor
BBConstraints::~BBConstraints()
{
  // Clean up resources if necessary
}

void BBConstraints::show_nodes_pumps_and_tanks(bool verbose)
{
  if (verbose)
  {
    std::cout << "\nNodes: [ ";
    for (const auto &node : nodes)
      std::cout << node.first << " ";
    std::cout << "]" << std::endl;

    std::cout << "Pumps: [ ";
    for (const std::string &pump_name : pump_names)
      std::cout << pump_name << " ";
    std::cout << "]" << std::endl;

    std::cout << "Tanks: [ ";
    for (const auto &tank : tanks)
      std::cout << tank.first << " ";
    std::cout << "]" << std::endl;
  }
}

void BBConstraints::get_nodes_and_tanks_ids(std::string inpFile)
{
  EN_Project p = EN_createProject();
  CHK(EN_loadProject(const_cast<char *>(inpFile.c_str()), p), "Load project");

  // Find node IDs
  for (auto &node : nodes)
  {
    const std::string &node_name = node.first;
    int node_id;
    CHK(EN_getNodeIndex(const_cast<char *>(node_name.c_str()), &node_id, p), "Get node index");
    node.second = node_id;
  }

  // Find tank IDs
  for (auto &tank : tanks)
  {
    const std::string &tank_name = tank.first;
    int tank_id;
    CHK(EN_getNodeIndex(const_cast<char *>(tank_name.c_str()), &tank_id, p), "Get tank index");
    tank.second = tank_id;
  }

  EN_deleteProject(p);
}

// Function to display pressure status
void BBConstraints::show_pressures(bool is_feasible, const std::string &node_name, double pressure, double threshold)
{
  if (!is_feasible)
    printf("  \u274C node[%3s]: %.2f < %.2f\n", node_name.c_str(), pressure, threshold);
  else
    printf("  \u2705 node[%3s]: %.2f >= %.2f\n", node_name.c_str(), pressure, threshold);
}

// Function to display tank level status
void BBConstraints::show_levels(bool is_feasible, const std::string &tank_name, double level, double level_min,
                                double level_max)
{
  if (!is_feasible)
    printf("  \u274C tank[%3s]: %.2f not in [%.2f, %.2f]\n", tank_name.c_str(), level, level_min, level_max);
  else
    printf("  \u2705 tank[%3s]: %.2f in [%.2f, %.2f]\n", tank_name.c_str(), level, level_min, level_max);
}

// Function to display stability status
void BBConstraints::show_stability(bool is_feasible, const std::string &tank_name, double level, double initial_level)
{
  if (!is_feasible)
    printf("  \u274C tank[%3s]: %.2f < %.2f\n", tank_name.c_str(), level, initial_level);
  else
    printf("  \u2705 tank[%3s]: %.2f >= %.2f\n", tank_name.c_str(), level, initial_level);
}

// Function to check node pressures
bool BBConstraints::check_pressures(bool verbose)
{
  if (verbose)
  {
    std::cout << "\nChecking pressures: [";
    for (const auto &node : nodes)
      std::cout << node.first << " ";
    std::cout << "]" << std::endl;
  }

  std::map<std::string, double> thresholds = {{"55", 42}, {"90", 51}, {"170", 30}};
  bool all_ok = true;

  for (const auto &node : nodes)
  {
    const std::string &node_name = node.first;
    const int &node_id = node.second;
    double pressure;

    // Retrieve node pressure
    CHK(EN_getNodeValue(node_id, EN_PRESSURE, &pressure, p), "Get node pressure");

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

// Function to display pattern information
void show_pattern(Pattern *p, const std::string &name)
{
  std::string type_name;
  switch (p->type)
  {
  case Pattern::FIXED_PATTERN:
    type_name = "FIXED";
    break;
  case Pattern::VARIABLE_PATTERN:
    type_name = "VARIABLE";
    break;
  default:
    type_name = "UNKNOWN";
    break;
  }
  std::cout << name << "[" << type_name << ", " << p->size() << "]: [";
  for (int i = 0; i < p->size(); i++)
  {
    std::cout << p->factor(i) << " ";
  }
  std::cout << "]" << std::endl;
}

// Function to update pump speed patterns based on counter
void BBConstraints::update_pumps(const int h, const std::vector<int> &x, bool verbose)
{
  size_t num_pumps = pumps.size();
  if (verbose)
  {
    printf("\nUpdating pumps speed: h=%d, num_pumps=%zu\n", h, num_pumps);
  }
  for (int i = 1; i <= h; i++)
  {
    for (size_t pump_id = 0; pump_id < num_pumps; pump_id++)
    {
      Pump *pump = pumps[pump_id];
      FixedPattern *pattern = dynamic_cast<FixedPattern *>(pump->speedPattern);
      if (!pattern)
      {
        std::cerr << "Error: Pump " << pump_id << " does not have a FixedPattern speed pattern." << std::endl;
        continue;
      }
      const int *xi = &x[num_pumps * i];
      double factor_new = static_cast<double>(xi[pump_id]);
      const int factor_id = i - 1; // pattern index is 0-based
      double factor_old = pattern->factor(factor_id);
      pattern->setFactor(factor_id, factor_new);
      if (verbose)
      {
        std::cout << "   h[" << i << "]: pump[" << pump_id << "]: " << factor_old << " -> " << factor_new << std::endl;
      }
    }
  }
}

// Function to retrieve pump objects from the network
void BBConstraints::set_pumps(Network *nw, const int h, const std::vector<int> &x, bool verbose)
{
  // Retrieve pump objects from the network
  for (const std::string &name : pump_names)
  {
    Pump *pump = dynamic_cast<Pump *>(nw->link(name));
    if (pump == nullptr)
      std::cout << "Pump " << name << " not found" << std::endl;
    else
    {
      pumps.push_back(pump);
    }
  }

  // Update pump speed patterns based on counter
  update_pumps(h, x, verbose);
}

// Function to check tank levels
bool BBConstraints::check_levels(bool verbose)
{
  if (verbose)
  {
    std::cout << "\nChecking levels: [";
    for (const auto &tank : tanks)
      std::cout << tank.first << " ";
    std::cout << "]" << std::endl;
  }

  const double level_min = 66.53;
  const double level_max = 71.53;
  bool all_ok = true;

  for (const auto &tank : tanks)
  {
    const std::string &tank_name = tank.first;
    const int &tank_id = tank.second;
    double level;

    // Retrieve tank level
    CHK(EN_getNodeValue(tank_id, EN_HEAD, &level, p), "Get tank level");

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
bool BBConstraints::check_stability(bool verbose)
{
  if (verbose)
  {
    std::cout << "\nChecking stability: [";
    for (const auto &tank : tanks)
      std::cout << tank.first << " ";
    std::cout << "]" << std::endl;
  }

  const double initial_level = 66.93;
  bool all_ok = true;

  for (const auto &tank : tanks)
  {
    const std::string &tank_name = tank.first;
    const int &tank_id = tank.second;
    double level;

    // Retrieve tank level
    CHK(EN_getNodeValue(tank_id, EN_HEAD, &level, p), "Get tank level");

    // Check if level meets the initial stability condition
    bool is_feasible = level >= initial_level;
    if (!is_feasible)
    {
      all_ok = false;
    }

    // Display stability status
    if (verbose) show_stability(is_feasible, tank_name, level, initial_level);
  }

  return all_ok;
}

// Function to check the cost
bool BBConstraints::check_cost(const double cost, bool verbose)
{
  bool is_feasible = cost < cost_max;
  if (verbose)
  {
    printf("Checking cost:\n");
    if (is_feasible)
      printf("  \u2705 cost=%.2f < cost_max=%.2g\n", cost, cost_max);
    else
      printf("  \u274C cost=%.2f >= cost_max=%.2g\n", cost, cost_max);
  }
  return is_feasible;
}

// Function to calculate the total cost of pump operations
double BBConstraints::calc_cost() const
{
  double cost = 0.0;
  for (Pump *pump : pumps)
  {
    cost += pump->pumpEnergy.getCost();
  }
  return cost;
}