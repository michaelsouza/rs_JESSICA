// src/CLI/BBConstraints.h
#pragma once

#include "CLI/Console.h"
#include "Core/network.h"
#include "Core/project.h"
#include "Elements/pump.h"
#include "epanet3.h"

#include <map>
#include <string>

using Epanet::Project;

/**
 * @brief Class to handle constraint checking for branch and bound optimization
 */
class BBConstraints
{
public:
  /**
   * @brief Constructs constraints checker for the given input file
   * @param inpFile Path to the EPANET input file
   */
  BBConstraints(std::string inpFile);
  ~BBConstraints();

  /**
   * @brief Verifies that node pressures meet minimum requirements
   * @param verbose If true, prints detailed constraint violation info
   * @return true if all pressure constraints are satisfied
   */
  bool check_pressures(Project *p, bool verbose = false);

  /**
   * @brief Verifies that tank levels are within bounds
   * @param verbose If true, prints detailed constraint violation info
   * @return true if all tank level constraints are satisfied
   */
  bool check_levels(Project *p, bool verbose = false);

  /**
   * @brief Verifies that final tank levels match initial levels
   * @param verbose If true, prints detailed constraint violation info
   * @return true if all tanks return to initial levels
   */
  bool check_stability(Project *p, bool verbose = false);

  /**
   * @brief Checks if total operational cost is within allowed bounds
   * @param cost The cost to check
   * @param verbose If true, prints detailed constraint violation info
   * @return true if cost is below maximum allowed
   */
  bool check_cost(Project *p, const double cost, bool verbose = false);

  /**
   * @brief Loads node and tank IDs from input file
   * @param inpFile Path to the EPANET input file
   */
  void get_network_elements_indices(std::string inpFile);

  /**
   * @brief Calculates total pump operation cost
   * @return Total operational cost
   */
  double calc_cost(Project *p) const;

  /**
   * @brief Gets number of nodes in network
   * @return Number of nodes
   */
  int get_num_nodes() const
  {
    return (int)nodes.size();
  }

  /**
   * @brief Gets number of tanks in network
   * @return Number of tanks
   */
  int get_num_tanks() const
  {
    return (int)tanks.size();
  }

  /**
   * @brief Gets number of pumps in network
   * @return Number of pumps
   */
  int get_num_pumps() const
  {
    return (int)pumps.size();
  }

  /**
   * @brief Displays constraint information
   */
  void show() const;

  /**
   * @brief Updates pump statuses for the initial time period until the current time period
   * @param p Project containing the network
   * @param h Current time period
   * @param x Vector of pump statuses (0/1)
   * @param verbose If true, prints status updates
   */
  void update_pumps(Project *p, const int h, const std::vector<int> &x, bool verbose);

  std::map<std::string, int> nodes;           ///< Map of node names to indices
  std::map<std::string, int> tanks;           ///< Map of tank names to indices
  std::map<std::string, int> pumps;           ///< Map of pump names to indices
  std::pair<int, std::vector<double>> prices; ///< PRICES pattern index and values
  double cost_ub;                             ///< Maximum cost allowed (upper bound)
  std::string inpFile;                        ///< Path to input file

private:
  /**
   * @brief Helper to display pressure constraint status
   * @param is_feasible Whether constraint is satisfied
   * @param node_name Name of node being checked
   * @param pressure Current pressure at node
   * @param threshold Minimum required pressure
   */
  void show_pressures(bool is_feasible, const std::string &node_name, double pressure, double threshold);

  /**
   * @brief Helper to display tank level constraint status
   * @param is_feasible Whether constraint is satisfied
   * @param tank_name Name of tank being checked
   * @param level Current tank level
   * @param level_min Minimum allowed level
   * @param level_max Maximum allowed level
   */
  void show_levels(bool is_feasible, const std::string &tank_name, double level, double level_min, double level_max);

  /**
   * @brief Helper to display tank stability constraint status
   * @param is_feasible Whether constraint is satisfied
   * @param tank_name Name of tank being checked
   * @param level Final tank level
   * @param initial_level Initial tank level
   */
  void show_stability(bool is_feasible, const std::string &tank_name, double level, double initial_level);
};
