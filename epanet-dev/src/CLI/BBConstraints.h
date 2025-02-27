// src/CLI/BBConstraints.h
#pragma once

#include "CLI/BBConfig.h"
#include "CLI/Console.h"

#include "Core/network.h"
#include "Core/project.h"
#include "Elements/pump.h"
#include "epanet3.h"

#include <map>
#include <mpi.h>
#include <queue>
#include <string>

using Epanet::Project;

enum BBPruneReason
{
  NONE,
  PRESSURES,
  LEVELS,
  STABILITY,
  COST,
  ACTUATIONS
};

/**
 * @brief Class to handle constraint checking for branch and bound optimization
 */
class BBConstraints
{
public:
  std::map<std::string, int> nodes; ///< Map of node names to indices
  std::map<std::string, int> tanks; ///< Map of tank names to indices
  std::map<std::string, int> pumps; ///< Map of pump names to indices
  std::string inpFile;              ///< Path to input file
  double best_cost_local;           ///< Local best cost
  double best_cost_global;          ///< Global best cost
  std::vector<int> best_x;          ///< Best pump statuses
  std::vector<int> best_y;          ///< Best pump speed patterns
  MPI_Request request_nonblocking;

  /**
   * @brief Synchronizes the best solution found among all processes
   */
  void sync_best();

  /**
   * @brief Constructs constraints checker for the given input file
   * @param inpFile Path to the EPANET input file
   */
  BBConstraints(const BBConfig &config);
  ~BBConstraints();

  /**
   * @brief Verifies that node pressures meet minimum requirements
   * @param verbose If true, prints detailed constraint violation info
   * @return true if all pressure constraints are satisfied
   */
  bool check_pressures(Project &p, bool verbose = false);

  /**
   * @brief Verifies that tank levels are within bounds
   * @param verbose If true, prints detailed constraint violation info
   * @return true if all tank level constraints are satisfied
   */
  bool check_levels(Project &p, bool verbose = false);

  /**
   * @brief Verifies that final tank levels match initial levels
   * @param verbose If true, prints detailed constraint violation info
   * @return true if all tanks return to initial levels
   */
  BBPruneReason check_stability(Project &p, bool verbose = false);

  /**
   * @brief Checks if total operational cost is within allowed bounds
   * @param cost The cost to check
   * @param verbose If true, prints detailed constraint violation info
   * @return true if cost is below maximum allowed
   */
  bool check_cost(Project &p, double &cost, bool verbose = false);

  /**
   * @brief Checks if the current state of the network is feasible
   * @param p Project containing the network
   * @param h Current time period
   * @param cost Cost of the current state
   * @return PruneType of the reason for pruning
   */
  BBPruneReason check_feasibility(Project &p, const int h, double &cost, bool verbose);

  /**
   * @brief Loads node and tank IDs from input file
   * @param inpFile Path to the EPANET input file
   */
  void get_network_elements_indices(std::string inpFile);

  /**
   * @brief Calculates total pump operation cost
   * @return Total operational cost
   */
  double calc_cost(Project &p) const;

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

  std::string fmt_cost(double cost) const
  {
    char fmt[256];
    if (cost > 999999999)
      sprintf(fmt, "INFINITY");
    else
      sprintf(fmt, "%.2f", cost);
    return fmt;
  }
  /**
   * @brief Displays the best solution found
   */
  void show_best() const;

  /**
   * @brief Updates pump statuses for the initial time period until the current time period
   * @param p Project containing the network
   * @param h Current time period
   * @param x Vector of pump statuses (0/1)
   * @param verbose If true, prints status updates
   */
  void update_pumps(Project &p, const int h, const std::vector<int> &x, bool verbose);

  /**
   * @brief Updates the best solution found
   * @param cost Cost of the new solution
   * @param x Pump statuses of the new solution
   * @param y Pump speed patterns of the new solution
   */
  void update_best(double cost, std::vector<int> x, std::vector<int> y);

  /**
   * @brief Writes the best solution to a JSON file
   * @param fn Path to the output file
   */
  void to_json(char *fn) const;

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
