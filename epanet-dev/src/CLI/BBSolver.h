// src/CLI/BBSolver.h
#pragma once

#include "BBConfig.h"
#include "BBConstraints.h"
#include "BBPruneReason.h"
#include "BBStats.h"

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

/**
 * @class BBCounter
 * @brief Manages the state and transitions of pump actuations in a water distribution network.
 *
 * The BBCounter class is responsible for tracking and updating the actuations
 * of pumps over a defined time horizon. It ensures that the number of actuations
 * does not exceed the specified maximum and provides utility functions to manipulate
 * and display the current state.
 */
class BBSolver
{
public:
  /**
   * @brief Constructs a BBCounter instance with specified parameters.
   *
   * @param config The configuration object.
   */
  BBSolver(BBConfig &config);

  void solve();

  /**
   * @brief Sets the y vector to the provided values.
   *
   * @param y A vector of integers representing the desired y states.
   * @return true if the y vector is set successfully, false otherwise.
   */
  bool set_y(const std::vector<int> &y);

  /**
   * @brief Processes a node in the branch and bound tree by simulating hydraulics and checking constraints
   *
   * @param cost Reference to store the total operating cost
   * @return true if node solution is feasible, false if any constraints violated
   */
  bool process_node(double &cost);

  /**
   * @brief Updates the y vector based on feasibility.
   *
   * @return true if the update is successful, false if there is no feasible state.
   */
  bool update_y();

  /**
   * @brief Updates the x vector based on the current y vector.
   *
   * @param verbose If true, prints detailed update information.
   * @return true if the update is successful and feasible, false otherwise.
   */
  bool update_x(bool verbose = false);

  /**
   * @brief Displays the current state of y and x vectors.
   *
   * @param verbose If true, prints detailed information to the console.
   */
  void show_xy(bool verbose = false) const;

  /**
   * @brief Jumps the current y state to its maximum value.
   */
  void jump_to_end();

  /**
   * @brief Retrieves the top level that is free for actuations.
   *
   * @return The top level index that can accept more actuations.
   */
  int get_free_level();

  /**
   * @brief Displays the current state of the counter.
   */
  void show(bool show_constraints) const;

  /**
   * @brief Fills a vector with the current state of the counter.
   *
   * @param recv_buffer The vector to fill with the current state.
   */
  void write_buffer();

  /**
   * @brief Reads the current state of the counter from a vector.
   *
   * @param recv_buffer The vector containing the current state.
   */
  void read_buffer();
  void add_prune(PruneReason reason);

  /**
   * @brief Updates the pump states in the EPANET project based on the current solution
   *
   * @param p The EPANET project to update
   * @param full_update If true, updates pumps for all time periods. If false, only updates current period
   */
  void update_pumps(Project &p, bool full_update);
  void add_feasible();
  bool try_split(const std::vector<int> &done, const std::vector<int> &h_free, int h_threshold, bool verbose);
  void update_cost_ub(double cost, bool update_xy);
  void to_json(double eta_secs);
  void send_work(int recv_rank, const std::vector<int> &h_free);
  void recv_work(int send_rank, const std::vector<int> &h_free);

  /**
   * @brief Core function to update the x vector.
   *
   * @param verbose If true, prints detailed update information.
   * @return true if the update is feasible, false otherwise.
   */
  bool update_x_h(bool verbose = false);
  void solve_iteration(int &done_loc, bool verbose, bool dump_project);
  void solve_sync(const int h_threshold, int &done_loc, int &done_all, bool verbose);

  int h;               ///< Current time period index.
  std::vector<int> y;  ///< Vector tracking actuations per time period.
  std::vector<int> x;  ///< Vector tracking pump states across time periods.
  int num_pumps;       ///< Number of pumps being managed.
  int h_min;           ///< Current top level in the counter.
  int h_cut;           ///< Threshold for top level operations.
  int &h_max;          ///< Total number of time periods.
  int &max_actuations; ///< Maximum actuations (turn off) allowed per pump.
  BBConstraints cntrs; ///< Constraints object for the network.
  int is_feasible;     ///< Indicates whether the current state is feasible (Using int to match MPI_INT).

  BBStats stats;   ///< Statistics object for tracking feasibility and pruning.
  BBConfig config; ///< Configuration object for solver parameters.

  // std::vector<nlohmann::json> snapshots;
  std::vector<ProjectData> snapshots;
  Project p;

  // MPI variables
  std::vector<int> mpi_buffer; ///< Buffer for receiving data from other ranks.
  int mpi_rank;                ///< Rank of the current process.
  int mpi_size;                ///< Size of the MPI communicator.
  std::vector<int> y_best;     ///< Best y vector found.
  std::vector<int> x_best;     ///< Best x vector found.

  void epanet_load();
  void epanet_solve(Project &p, double &cost);
};
