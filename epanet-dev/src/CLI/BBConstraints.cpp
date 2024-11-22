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
  this->pumps = {{"111", nullptr}, {"222", nullptr}, {"333", nullptr}};
  this->cost_ub = std::numeric_limits<double>::max();

  // Retrieve node and tank IDs from the input file
  get_nodes_tanks_ids(inpFile);
}

// Destructor
BBConstraints::~BBConstraints()
{
  // Clean up resources if necessary
}

void BBConstraints::show() const
{
  Console::hline(Console::Color::BRIGHT_WHITE);
  Console::printf(Console::Color::BRIGHT_WHITE, "BBConstraints\n");
  Console::printf(Console::Color::BRIGHT_WHITE, "Nodes: [ ");
  for (const auto &node : nodes)
    Console::printf(Console::Color::BRIGHT_WHITE, "%s ", node.first.c_str());
  Console::printf(Console::Color::BRIGHT_WHITE, "]\n");

  Console::printf(Console::Color::BRIGHT_WHITE, "Pumps: [ ");
  for (const auto &pump : pumps)
    Console::printf(Console::Color::BRIGHT_WHITE, "%s ", pump.first.c_str());
  Console::printf(Console::Color::BRIGHT_WHITE, "]\n");

  Console::printf(Console::Color::BRIGHT_WHITE, "Tanks: [ ");
  for (const auto &tank : tanks)
    Console::printf(Console::Color::BRIGHT_WHITE, "%s ", tank.first.c_str());
  Console::printf(Console::Color::BRIGHT_WHITE, "]\n");
}

void BBConstraints::get_nodes_tanks_ids(std::string inpFile)
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
    Console::printf(Console::Color::RED, "  \u274C node[%3s]: %.2f < %.2f\n", node_name.c_str(), pressure, threshold);
  else
    Console::printf(Console::Color::GREEN, "  \u2705 node[%3s]: %.2f >= %.2f\n", node_name.c_str(), pressure, threshold);
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
bool BBConstraints::check_pressures(bool verbose)
{
  if (verbose)
  {
    Console::printf(Console::Color::BRIGHT_WHITE, "\nChecking pressures: [");
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

// Function to check tank levels
bool BBConstraints::check_levels(bool verbose)
{
  if (verbose)
  {
    Console::printf(Console::Color::BRIGHT_WHITE, "\nChecking levels: [");
    for (const auto &tank : tanks)
      Console::printf(Console::Color::BRIGHT_WHITE, "%s ", tank.first.c_str());
    Console::printf(Console::Color::BRIGHT_WHITE, "]\n");
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
  bool is_feasible = cost < cost_ub;
  if (verbose)
  {
    Console::printf(Console::Color::BRIGHT_WHITE, "\nChecking cost:\n");
    std::string cost_max_str = (cost_ub > 999999999) ? "inf" : std::to_string(cost_ub);
    if (is_feasible)
      Console::printf(Console::Color::GREEN, "  \u2705 cost=%.2f < cost_max=%s\n", cost, cost_max_str.c_str());
    else
      Console::printf(Console::Color::RED, "  \u274C cost=%.2f >= cost_max=%s\n", cost, cost_max_str.c_str());
  }
  return is_feasible;
}

// Function to calculate the total cost of pump operations
double BBConstraints::calc_cost() const
{
  double cost = 0.0;
  for (const auto &pump : pumps)
  {
    cost += pump.second->pumpEnergy.getCost();
  }
  return cost;
}

void BBConstraints::update_pumps(EN_Project p, const int h, const std::vector<int> &x, bool verbose)
{
  if (verbose) Console::printf(Console::Color::BRIGHT_WHITE, "\nUpdating pumps\n");

  // Set project pointer. This is used by the "check" methods.
  this->p = p;

  // Get project network
  Project *prj = static_cast<Project *>(p);
  Network *nw = prj->getNetwork();

  // Find pumps
  for (auto &pump : pumps)
  {
    const std::string pump_name = pump.first;
    Pump *link = (Pump *)nw->link(pump_name);
    if (!link)
    {
      Console::printf(Console::Color::RED, "  The pump %s could not be found.\n", pump_name);
      exit(EXIT_FAILURE);
    }
    // Update pointer
    pump.second = link;
  }

  // Update pump speed patterns based on vector x
  int j = 0;
  const size_t num_pumps = get_num_pumps();
  for (int i = 1; i <= h; i++)
  {
    const int *xi = &x[num_pumps * i];
    j = 0;
    for (auto &pump : pumps)
    {
      const auto &pump_name = pump.first;
      const auto &pump_link = pump.second;
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
      double factor_old = pattern->factor(factor_id);
      // Update speed factor
      pattern->setFactor(factor_id, factor_new);

      if (verbose)
      {
        Console::printf(Console::Color::CYAN, "  h[%d]: pump[%s]: %.0f -> %.0f\n", i, pump_name.c_str(), factor_old, factor_new);
      }
    }
  }
}
