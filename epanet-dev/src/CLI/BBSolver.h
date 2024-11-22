// src/CLI/BBSolver.h
#pragma once

#include "BBConstraints.h"
#include "BBPruneReason.h"
#include "BBStats.h"

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
   * @param inpFile The input file name.
   * @param h_max The total number of time periods (hours) to manage.
   * @param max_actuations The maximum number of actuations permitted for each pump.
   */
  BBSolver(std::string inpFile, int h_max, int max_actuations);

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
   * @brief Sets the y vector to the provided values.
   *
   * @param y A vector of integers representing the desired y states.
   * @return true if the y vector is set successfully, false otherwise.
   */
  bool set_y(const std::vector<int> &y);

  /**
   * @brief Retrieves the top level that is free for actuations.
   *
   * @return The top level index that can accept more actuations.
   */
  int get_free_level();

  /**
   * @brief Displays the current state of the counter.
   */
  void show() const;

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

  bool process_node(double &cost, bool verbose, bool save_project);

  void update_pumps(EN_Project p, bool verbose);

  void add_feasible();

  bool try_split(const std::vector<int> &done, const std::vector<int> &h_free, int h_threshold, bool verbose);

  void update_cost(double cost, bool update_xy);

  // Public member variables
  int h;                       ///< Current time period index.
  int h_max;                   ///< Total number of time periods.
  std::vector<int> y;          ///< Vector tracking actuations per time period.
  std::vector<int> x;          ///< Vector tracking pump states across time periods.
  int max_actuations;          ///< Maximum actuations allowed per pump.
  int num_pumps;               ///< Number of pumps being managed.
  int h_min;                   ///< Current top level in the counter.
  int h_cut;                   ///< Threshold for top level operations.
  std::string inpFile;         ///< Input file name.
  BBConstraints cntrs;         ///< Constraints object for the network.
  std::vector<int> mpi_buffer; ///< Buffer for receiving data from other ranks.
  int mpi_rank;                ///< Rank of the current process.
  int mpi_size;                ///< Size of the MPI communicator.
  std::vector<int> y_best;     ///< Best y vector found.
  std::vector<int> x_best;     ///< Best x vector found.
  int is_feasible;             ///< Indicates whether the current state is feasible (Using int to match MPI_INT).
  BBStats stats;               ///< Statistics object for tracking feasibility and pruning.

private:
  void send_work(int recv_rank, const std::vector<int> &h_free, bool verbose);

  void recv_work(int send_rank, const std::vector<int> &h_free, bool verbose);

  /**
   * @brief Core function to update the x vector.
   *
   * @param verbose If true, prints detailed update information.
   * @return true if the update is feasible, false otherwise.
   */
  bool update_x_core(bool verbose = false);

  /**
   * @brief Calculates the cumulative sum of actuations for each pump up to a given time.
   *
   * @param actuations_csum Array to store the cumulative actuations per pump.
   * @param x The current state vector of pump actuations.
   * @param h The current time period index.
   */
  void calc_actuations_csum(int *actuations_csum, const std::vector<int> &x, int h);
};

void solve(int argc, char *argv[]);
