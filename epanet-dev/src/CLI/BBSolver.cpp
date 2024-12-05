// src/CLI/BBSolver.cpp
#include "BBSolver.h"
#include "Console.h"
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <mpi.h>
#include <numeric>
#include <stdexcept>
#include <thread>

BBSolver::BBSolver(BBConfig &config)
    : cntrs(config.inpFile), stats(config.h_max), config(config), h_max(config.h_max), max_actuations(config.max_actuations)
{
  // Initialize scalar variables
  num_pumps = cntrs.get_num_pumps();
  h = 0;
  h_cut = 0;
  h_min = 0;
  is_feasible = true;

  // Initialize y and x vectors
  y = std::vector<int>(h_max + 1, 0);
  x = std::vector<int>(num_pumps * (h_max + 1), 0);
  y_best = std::vector<int>(h_max + 1, 0);
  x_best = std::vector<int>(num_pumps * (h_max + 1), 0);

  // Initialize the project
  // CHK(p.load(this->inpFile.c_str()), "Load project");

  // Allocate the recv buffer
  mpi_buffer.resize(4 + y.size() + x.size());
  MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
}

bool BBSolver::process_node(double &cost, bool verbose, bool save_project)
{
  is_feasible = true;
  int t = 0, dt = 0, t_max = 3600 * h_max;
  bool pumps_update_full = true;

  Project p;
  CHK(p.load(config.inpFile.c_str()), "Load project");
  CHK(p.initSolver(EN_INITFLOW), "Initialize solver");

  // Set the project and constraints
  update_pumps(p, pumps_update_full, verbose);

  if (verbose) show(true);

  do
  {
    // Run the solver
    CHK(p.runSolver(&t), "Run solver");

    // Advance the solver
    CHK(p.advanceSolver(&dt), "Advance solver");

    // Check cost
    cost = cntrs.calc_cost();
    is_feasible = cntrs.check_cost(cost, verbose);
    if (!is_feasible)
    {
      add_prune(PruneReason::COST);
      jump_to_end();
      break;
    }

    const int t_new = t + dt;

    // Early stop if the t_max is reached for non-final hours
    if (t_new > t_max && h != h_max) break;

    // Show the current state
    if (verbose) Console::printf(Console::Color::MAGENTA, "\nSimulation: t_new=%d, t_max=%d, t=%d, dt=%d, cost=%.2f\n", t_new, t_max, t, dt, cost);

    // Check node pressures
    is_feasible = cntrs.check_pressures(verbose);
    if (!is_feasible)
    {
      add_prune(PruneReason::PRESSURES);
      break;
    }

    // Check tank levels
    is_feasible = cntrs.check_levels(verbose);
    if (!is_feasible)
    {
      add_prune(PruneReason::LEVELS);
      break;
    }

  } while (dt > 0);

  // Show the final state
  if (verbose) Console::printf(Console::Color::MAGENTA, "\nSimulation: t_max=%d, t=%d, dt=%d\n, cost=%.2f", t_max, t, dt, cost);

  // Check stability for the last hour
  if (is_feasible && h == h_max)
  {
    is_feasible = cntrs.check_stability(verbose);
    if (!is_feasible) add_prune(PruneReason::STABILITY);
  }

  if (save_project)
  {
    // Create a file with timestamp based name
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << "output_" << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S") << ".inp";
    p.save(ss.str().c_str());
    Console::printf(Console::Color::BRIGHT_GREEN, "Project saved to: %s\n", ss.str().c_str());
  }

  return is_feasible;
}

void BBSolver::update_pumps(Project &p, bool full_update, bool verbose)
{
  if (full_update)
  {
    // Update pumps for all time periods
    for (int i = 0; i <= h_max; ++i)
    {
      cntrs.update_pumps(p, i, x, verbose);
    }
  }
  else
  {
    // Update pumps only for current time period
    cntrs.update_pumps(p, this->h, this->x, verbose);
  }
}

bool BBSolver::set_y(const std::vector<int> &y)
{
  // Copy y to the internal y vector
  this->y = y;

  // Start assuming the y vector is feasible
  is_feasible = true;

  // Set y and update x for all hours
  h = 0;
  for (int i = 0; i < h_max; ++i)
  {
    h++;
    bool updated = update_x(false);
    if (!updated) return false;
  }

  return true;
}

bool BBSolver::update_y()
{
  if (h < h_min || h > h_max)
  {
    // pass: invalid hour
  }
  else if (is_feasible)
  {
    if (h < h_max)
    {
      y[++h] = 0;
      return true;
    }

    // If the current level is the last level, there is no more work to do
    if (h == h_max)
    {
      if (h == h_min)
      {
        // There is no more work to do
        if (y[h] == h_cut) return false;

        // Otherwise, increment the current level
        y[h]++;
        return true;
      }

      if (y[h] < num_pumps)
      {
        y[h]++;
        return true;
      }

      // Otherwise, decrement the current level
      --h;
      is_feasible = false;
      return update_y();
    }
  }
  else // The current state is infeasible
  {
    if (h == h_min)
    {
      // Increment the current level
      if (y[h] < h_cut)
      {
        y[h]++;
        return true;
      }

      // There is no more work to do
      if (y[h] == h_cut) return false;
    }

    if (h_min < h && h <= h_max)
    {
      // The level is finished
      if (y[h] == num_pumps)
      {
        --h;
        return update_y();
      }

      // Otherwise, increment the current level
      y[h]++;
      return true;
    }
  }
  // Raise an error if the current level is not in the proper range
  Console::printf(Console::Color::RED, "ERR[rank=%d]: is_feasible=%d, h=%d, ", mpi_rank, is_feasible, h);
  Console::printf(Console::Color::RED, "y[h]=%d, [h_min=%d, h_max=%d, h_cut=%d] are incompatible in update_y\n", y[h], h_min, h_max, h_cut);
  MPI_Abort(MPI_COMM_WORLD, 1);
  return false;
}

bool BBSolver::update_x(bool verbose)
{
  // Update x core
  is_feasible = this->update_x_h(verbose);

  // Check consistency sum(x[h]) == y[h]
  if (is_feasible)
  {
    const int *x_new = &x[num_pumps * h];
    int sum_x = std::accumulate(x_new, x_new + num_pumps, 0);
    if (sum_x != y[h]) throw std::runtime_error("sum(x)=" + std::to_string(sum_x) + " != y=" + std::to_string(y[h]));
  }

  return is_feasible;
}

bool switch_pumps_off(int *x_new, const std::vector<int> &pumps_sorted, const std::vector<int> &allowed_10, int &counter_10)
{
  for (const int pump_id : pumps_sorted)
  {
    if (counter_10 <= 0) break;

    if (x_new[pump_id] == 1)
    {
      if (allowed_10[pump_id] <= 0) return false; // Cannot turn off this pump

      x_new[pump_id] = 0; // Turn off the pump
      --counter_10;
    }
  }
  return counter_10 == 0;
}

void compute_allowed_switches(const int num_pumps, const int *x, int current_h, std::vector<int> &allowed_01, std::vector<int> &allowed_10)
{
  for (int pump_id = 0; pump_id < num_pumps; ++pump_id)
  {
    for (int i = 2; i < current_h; ++i)
    {
      int x_old = x[pump_id + num_pumps * (i - 1)];
      int x_new = x[pump_id + num_pumps * i];
      if (x_old < x_new)
      {
        // Pump was turned on (0 -> 1)
        --allowed_01[pump_id];
      }
      else if (x_old > x_new)
      {
        // Pump was turned off (1 -> 0)
        --allowed_10[pump_id];
      }
    }
  }
}

void sort_pumps(std::vector<int> &pumps_sorted, const std::vector<int> &allowed_01, const std::vector<int> &allowed_10, bool switch_on)
{
  if (switch_on)
  {
    // Sort pumps by decreasing (allowed_01, allowed_10)
    std::sort(pumps_sorted.begin(), pumps_sorted.end(),
              [&allowed_01, &allowed_10](int a, int b)
              {
                if (allowed_01[a] != allowed_01[b])
                {
                  return allowed_01[a] > allowed_01[b];
                }
                return allowed_10[a] > allowed_10[b];
              });
  }
  else
  {
    // Sort pumps by decreasing (allowed_10, allowed_01)
    std::sort(pumps_sorted.begin(), pumps_sorted.end(),
              [&allowed_01, &allowed_10](int a, int b)
              {
                if (allowed_10[a] != allowed_10[b])
                {
                  return allowed_10[a] > allowed_10[b];
                }
                return allowed_01[a] > allowed_01[b];
              });
  }
}

bool switch_pumps_on(int *x_new, const std::vector<int> &pumps_sorted, const std::vector<int> &allowed_01, int &counter_01)
{
  for (const int pump_id : pumps_sorted)
  {
    if (counter_01 <= 0) break;

    if (x_new[pump_id] == 0)
    {
      if (allowed_01[pump_id] <= 0) return false; // Cannot turn on this pump

      x_new[pump_id] = 1; // Turn on the pump
      --counter_01;
    }
  }
  return counter_01 == 0;
}

bool BBSolver::update_x_h(bool verbose)
{
  // Get the previous and new states
  const int &y_old = y[h - 1];
  const int &y_new = y[h];
  const int *x_old = &x[num_pumps * (h - 1)];
  int *x_new = &x[num_pumps * h];

  // Start by copying the previous state
  std::copy(x_old, x_old + this->num_pumps, x_new);

  // Nothing to be done
  if (y_new == y_old) return true;

  // Initialize allowed switches
  std::vector<int> allowed_01(num_pumps, max_actuations);
  std::vector<int> allowed_10(num_pumps, max_actuations);

  // Compute allowed switches based on history
  compute_allowed_switches(num_pumps, &x[0], h, allowed_01, allowed_10);

  // Create and initialize pump indices
  std::vector<int> pumps_sorted(num_pumps);
  for (int pump_id = 0; pump_id < num_pumps; ++pump_id)
  {
    pumps_sorted[pump_id] = pump_id;
  }

  bool success = true;
  if (y_new > y_old) // Switch on pumps
  {
    sort_pumps(pumps_sorted, allowed_01, allowed_10, true);
    int counter_01 = y_new - y_old;
    success = switch_pumps_on(x_new, pumps_sorted, allowed_01, counter_01);
  }
  else if (y_new < y_old) // Switch off pumps
  {
    sort_pumps(pumps_sorted, allowed_01, allowed_10, false);
    int counter_10 = y_old - y_new;
    success = switch_pumps_off(x_new, pumps_sorted, allowed_10, counter_10);
  }

  if (verbose)
  {
    Console::printf(Console::Color::BRIGHT_MAGENTA, "Rank[%d]: update_x_h[%d]: success=%d, y_new=%d, y_old=%d\n", mpi_rank, h, success, y_new, y_old);
    show_vector(x_new, num_pumps, "   x_new");
  }
  return success;
}

void BBSolver::jump_to_end()
{
  if (h == h_min)
    y[h] = h_cut;
  else
    y[h] = num_pumps;
}

int BBSolver::get_free_level()
{
  // Return the top level if its value is lower than the top cut
  if (y[h_min] < h_cut)
  {
    return h_min;
  }

  // Consider only the levels in the range [top_level + 1, h]
  for (int level = h_min + 1; level <= h; level++)
  {
    // For the remaining levels, the cut is the number of pumps
    if (y[level] < num_pumps)
    {
      // Update the top level and top cut
      h_min = level;
      h_cut = num_pumps;

      return h_min;
    }
  }

  // If no free level is found, return the last level
  return h_max;
}

/** Update Reason functions */
void BBSolver::add_prune(PruneReason reason)
{
  stats.add_pruning(reason, h);
}

void BBSolver::add_feasible()
{
  is_feasible = true;
  stats.add_feasible(h);
}

/** MPI functions */
void BBSolver::write_buffer()
{
  // Retrieve the MPI rank
  int rank = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  // Resize the buffer
  const size_t num_scalars = 4;
  mpi_buffer.resize(num_scalars + y.size() + x.size());

  // Write scalar values
  mpi_buffer[0] = h_min;
  mpi_buffer[1] = y[h_min]; // send the current top level value as top_cut
  mpi_buffer[2] = h;
  mpi_buffer[3] = is_feasible;

  // Write y vector
  std::copy(y.begin(), y.end(), mpi_buffer.begin() + num_scalars);

  // Write x vector
  std::copy(x.begin(), x.end(), mpi_buffer.begin() + num_scalars + y.size());
}

void BBSolver::read_buffer()
{
  // Retrieve the MPI rank
  int rank = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  // Read scalar values
  const size_t num_scalars = 4;
  h_min = mpi_buffer[0];
  h_cut = mpi_buffer[1];
  h = mpi_buffer[2];
  is_feasible = mpi_buffer[3];

  // Read y vector
  std::copy(mpi_buffer.begin() + num_scalars, mpi_buffer.begin() + num_scalars + y.size(), y.begin());

  // Read x vector
  std::copy(mpi_buffer.begin() + num_scalars + y.size(), mpi_buffer.begin() + num_scalars + y.size() + x.size(), x.begin());
}

void BBSolver::send_work(int recv_rank, const std::vector<int> &h_free, bool verbose)
{
  if (verbose) Console::printf(Console::Color::BRIGHT_MAGENTA, "Rank[%d]: Sending to rank %d\n", mpi_rank, recv_rank);

  // Write the buffer
  write_buffer();

  // Send the buffer
  auto mpi_error = MPI_Send(mpi_buffer.data(), mpi_buffer.size(), MPI_INT, recv_rank, 0, MPI_COMM_WORLD);
  if (mpi_error != MPI_SUCCESS)
  {
    if (verbose) Console::printf(Console::Color::RED, "Rank[%d]: MPI_Send failed with error code %d.\n", mpi_rank, mpi_error);
    MPI_Abort(MPI_COMM_WORLD, mpi_error);
  }

  // Update status
  h = h_min;
  is_feasible = false;
  add_prune(PruneReason::SPLIT);

  // Show the current state
  // if (verbose) show();
}

void BBSolver::recv_work(int send_rank, const std::vector<int> &h_free, bool verbose)
{
  if (verbose) Console::printf(Console::Color::BRIGHT_MAGENTA, "Rank[%d]: Receiving from rank %d\n", mpi_rank, send_rank);

  // Receive the buffer
  auto mpi_error = MPI_Recv(mpi_buffer.data(), mpi_buffer.size(), MPI_INT, send_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  if (mpi_error != MPI_SUCCESS)
  {
    if (verbose) Console::printf(Console::Color::RED, "Rank[%d]: MPI_Recv failed with error code %d.\n", mpi_rank, mpi_error);
    MPI_Abort(MPI_COMM_WORLD, mpi_error);
  }

  // Read the buffer
  read_buffer();

  // Show the current state
  // if (verbose) show();
}

bool BBSolver::try_split(const std::vector<int> &done, const std::vector<int> &h_free, int h_threshold, bool verbose)
{
  // Count the number of sending ranks
  int count_send = 0, count_recv = 0;
  for (int send_rank = 0; send_rank < mpi_size; ++send_rank)
  {
    // Skip if the rank is done, it has nothing to send
    if (done[send_rank]) continue;

    // Skip if there is not enough work to send (high level)
    if (h_free[send_rank] > h_threshold) continue;

    // Count the number of sending ranks
    ++count_send;

    // Reset the number of receiving ranks
    count_recv = 0;
    for (int recv_rank = 0; recv_rank < mpi_size; ++recv_rank)
    {
      // Consider only ranks that are done
      if (!done[recv_rank]) continue;

      // Count the number of receiving ranks
      ++count_recv;

      // Avoid sending to the same rank
      if (count_recv != count_send) continue;

      // This rank is a sender
      if (send_rank == mpi_rank)
      {
        send_work(recv_rank, h_free, verbose);
        return true;
      }

      // This rank is a receiver
      if (recv_rank == mpi_rank)
      {
        recv_work(send_rank, h_free, verbose);
        return true;
      }
    }
  }

  return false;
}

/** Show functions */
void BBSolver::show_xy(bool verbose) const
{
  if (!verbose) return;
  std::cout << "\n";
  for (int i = 1; i <= this->h; i++)
  {
    Console::printf(Console::Color::BRIGHT_WHITE, "h[%2d]: y=%d, x=[ ", i, this->y[i]);
    const int *x_i = &this->x[this->num_pumps * i];
    for (int j = 0; j < this->num_pumps; j++)
    {
      Console::printf(Console::Color::YELLOW, "%d ", x_i[j]);
    }
    Console::printf(Console::Color::BRIGHT_WHITE, "]\n");
  }
}

void BBSolver::show(bool show_constraints) const
{
  // Get MPI rank
  int rank = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  // Print Header with MPI rank
  Console::hline(Console::Color::BRIGHT_CYAN);
  Console::printf(Console::Color::BRIGHT_CYAN, "BBSolver (Rank %d)\n", rank);

  // Display Current Time Period
  Console::printf(Console::Color::YELLOW, "   h_min=%d, h_max=%d, h_cut=%d\n", h_min, h_max, h_cut);
  Console::printf(Console::Color::MAGENTA, "   h=%d, is_feasible=%d\n", h, is_feasible);

  // Display y and x vectors
  show_xy(true);

  // Display constraints
  if (show_constraints) cntrs.show();

  // Print Footer
  Console::printf(Console::Color::BRIGHT_CYAN, "\n");
}

void BBSolver::to_json(double eta_secs)
{
  stats.to_json(config, cntrs, eta_secs, y_best, x_best);
}

/** Main solve functions */
void parse_args(int argc, char *argv[], bool &verbose, int &h_max, int &max_actuations, bool &save_project, bool &use_logger, int &h_threshold)
{
  // Parse command line arguments
  for (int i = 1; i < argc; ++i)
  {
    std::string arg = argv[i];
    if (arg == "-v" || arg == "--verbose")
      verbose = true;
    else if (arg == "-h" || arg == "--h_max")
      h_max = std::stoi(argv[++i]);
    else if (arg == "-a" || arg == "--max_actuations")
      max_actuations = std::stoi(argv[++i]);
    else if (arg == "-s" || arg == "--save")
      save_project = true;
    else if (arg == "-l" || arg == "--log")
      use_logger = true;
    else if (arg == "-t" || arg == "--h_threshold")
      h_threshold = std::stoi(argv[++i]);
  }
}

void BBSolver::update_cost_ub(double cost, bool update_xy)
{
  if (cost > cntrs.cost_ub)
  {
    Console::printf(Console::Color::RED, "ERR[rank=%d]: cost=%.2f > cost_max=%.2f\n", mpi_rank, cost, cntrs.cost_ub);
    MPI_Abort(MPI_COMM_WORLD, 1);
  }

  if (cntrs.cost_ub > 999999999)
  {
    Console::printf(Console::Color::GREEN, "\nRank[%d]: 💰 updated cost_ub=%.2f (inf) %s\n", mpi_rank, cost, update_xy ? "new" : "");
  }
  else
  {
    Console::printf(Console::Color::GREEN, "\nRank[%d]: 💰 updated cost_ub=%.2f (%.2f) %s\n", mpi_rank, cost, cntrs.cost_ub, update_xy ? "new" : "");
  }

  cntrs.cost_ub = cost;

  if (update_xy)
  {
    y_best = y;
    x_best = x;
    Console::printf(Console::Color::BRIGHT_GREEN, "Rank[%d]: y = {", mpi_rank);
    for (size_t i = 0; i < y.size(); ++i)
    {
      Console::printf(Console::Color::BRIGHT_GREEN, "%d", y[i]);
      if (i < y.size() - 1) Console::printf(Console::Color::BRIGHT_GREEN, ", ");
    }
    Console::printf(Console::Color::BRIGHT_GREEN, "}\n");

    Console::printf(Console::Color::BRIGHT_GREEN, "Rank[%d]: x = {", mpi_rank);
    for (size_t i = 0; i < y.size(); ++i)
    {
      Console::printf(Console::Color::BRIGHT_GREEN, "{");
      for (int j = 0; j < num_pumps; ++j)
      {
        Console::printf(Console::Color::BRIGHT_GREEN, "%d", x[i * num_pumps + j]);
        if (j < num_pumps - 1) Console::printf(Console::Color::BRIGHT_GREEN, ", ");
      }
      Console::printf(Console::Color::BRIGHT_GREEN, "}");
      if (i < y.size() - 1) Console::printf(Console::Color::BRIGHT_GREEN, ", ");
    }
    Console::printf(Console::Color::BRIGHT_GREEN, "}\n");
  }
}

void BBSolver::solve_iteration(int &done_loc, bool verbose, bool save_project)
{
  if (done_loc) return;

  double cost = 0.0;

  // Update y vector
  done_loc = !update_y();
  if (done_loc)
  {
    if (verbose) Console::printf(Console::Color::BRIGHT_RED, "\nRank[%d]: done_loc=true\n", mpi_rank);
    return;
  }

  // Update x vector
  update_x(verbose);
  if (!is_feasible)
  {
    add_prune(PruneReason::ACTUATIONS);
    return;
  }

  // Process node
  process_node(cost, false, save_project);

  // Update feasible counter
  if (is_feasible)
  {
    add_feasible();

    // New solution found
    if (h == h_max)
    {
      update_cost_ub(cost, true);
    }
  }
}

void BBSolver::solve_sync(int h_threshold, int &done_loc, int &done_all, bool verbose)
{
  static int num_calls = 0;
  num_calls++;

  if (verbose)
  {
    Console::hline(Console::Color::BRIGHT_CYAN);
    Console::printf(Console::Color::BRIGHT_CYAN, "Rank[%d]: solve_sync #%d (done_loc=%d)\n", mpi_rank, num_calls, done_loc);
  }

  int mpi_error;
  std::vector<int> done(mpi_size, 0);
  std::vector<int> h_free(mpi_size, 0);
  std::vector<double> cost_ub(mpi_size, 0.0);

  // Synchronize cost_best across all ranks ================================
  mpi_error = MPI_Allgather(&cntrs.cost_ub, 1, MPI_DOUBLE, cost_ub.data(), 1, MPI_DOUBLE, MPI_COMM_WORLD);
  if (mpi_error != MPI_SUCCESS) MPI_Abort(MPI_COMM_WORLD, mpi_error);

  double cost_min = std::numeric_limits<double>::infinity();
  int rank_min = 0;
  for (int rank = 0; rank < mpi_size; ++rank)
  {
    if (cost_ub[rank] < cost_min)
    {
      cost_min = cost_ub[rank];
      rank_min = rank;
    }
  }

  if (cntrs.cost_ub > cost_min)
  {
    update_cost_ub(cost_min, false);
  }

  // Broadcast the solution from the rank with the minimum cost
  mpi_error = MPI_Bcast(y_best.data(), y_best.size(), MPI_INT, rank_min, MPI_COMM_WORLD);
  if (mpi_error != MPI_SUCCESS)
  {
    Console::printf(Console::Color::RED, "Rank[%d]: MPI_Bcast failed with error code %d.\n", mpi_rank, mpi_error);
    MPI_Abort(MPI_COMM_WORLD, mpi_error);
  }
  mpi_error = MPI_Bcast(x_best.data(), x_best.size(), MPI_INT, rank_min, MPI_COMM_WORLD);
  if (mpi_error != MPI_SUCCESS)
  {
    Console::printf(Console::Color::RED, "Rank[%d]: MPI_Bcast failed with error code %d.\n", mpi_rank, mpi_error);
    MPI_Abort(MPI_COMM_WORLD, mpi_error);
  }

  // Synchronize done_all across all ranks =================================
  mpi_error = MPI_Allgather(&done_loc, 1, MPI_INT, done.data(), 1, MPI_INT, MPI_COMM_WORLD);
  if (mpi_error != MPI_SUCCESS) MPI_Abort(MPI_COMM_WORLD, mpi_error);

  // Check if all ranks are done, if so, break
  done_all = std::all_of(done.begin(), done.end(), [](int i) { return i == 1; });
  if (done_all) return;

  // Update h_free with data from all ranks ================================
  int h_free_loc = get_free_level();
  mpi_error = MPI_Allgather(&h_free_loc, 1, MPI_INT, h_free.data(), 1, MPI_INT, MPI_COMM_WORLD);
  if (mpi_error != MPI_SUCCESS) MPI_Abort(MPI_COMM_WORLD, mpi_error);

  // The work is not done if the task has been split
  bool done_split = try_split(done, h_free, h_threshold, verbose);
  if (done_loc) done_loc = !done_split;

  if (verbose)
  {
    Console::printf(Console::Color::BRIGHT_MAGENTA, "Rank[%d]: MPI_Barrier\n", mpi_rank);
  }

  MPI_Barrier(MPI_COMM_WORLD);
}

void show_input_args(const char *inpFile, int h_max, int max_actuations, int h_threshold, bool verbose, bool save_project, bool use_logger)
{
  // Get MPI rank
  int rank = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  if (rank == 0)
  {
    Console::hline(Console::Color::BRIGHT_WHITE);
    Console::printf(Console::Color::BRIGHT_WHITE, "Input Arguments\n");
    Console::printf(Console::Color::WHITE, "   Input file: ");
    Console::printf(Console::Color::BRIGHT_GREEN, "%s\n", inpFile);
    Console::printf(Console::Color::WHITE, "   Time horizon (h_max): ");
    Console::printf(Console::Color::BRIGHT_GREEN, "%d\n", h_max);
    Console::printf(Console::Color::WHITE, "   Max actuations: ");
    Console::printf(Console::Color::BRIGHT_GREEN, "%d\n", max_actuations);
    Console::printf(Console::Color::WHITE, "   Split threshold: ");
    Console::printf(Console::Color::BRIGHT_GREEN, "%d\n", h_threshold);
    Console::printf(Console::Color::WHITE, "   Verbose: ");
    Console::printf(Console::Color::BRIGHT_GREEN, "%s\n", verbose ? "true" : "false");
    Console::printf(Console::Color::WHITE, "   Save project: ");
    Console::printf(Console::Color::BRIGHT_GREEN, "%s\n", save_project ? "true" : "false");
    Console::printf(Console::Color::WHITE, "   Use logger: ");
    Console::printf(Console::Color::BRIGHT_GREEN, "%s\n", use_logger ? "true" : "false");
    Console::hline(Console::Color::BRIGHT_WHITE);
  }

  // Sleep for 1 second
  std::this_thread::sleep_for(std::chrono::seconds(1));
}

void BBSolver::solve()
{
  // Get MPI rank and size
  int rank = mpi_rank;

  // Open logger file for each rank
  Console::open(rank, config.use_logger, config.verbose);

  // Initialize iteration variables
  int done_loc = (rank != 0); // Only rank 0 starts
  int done_all = 0;
  auto tic = std::chrono::high_resolution_clock::now();
  int niters = 0;

  // Main loop
  while (!done_all)
  {
    ++niters;

    show_timer(rank, niters, h, done_loc, done_all, cntrs.cost_ub, y_best, is_feasible, tic, 256, 1024);
    solve_iteration(done_loc, config.verbose, config.save_project);
    solve_sync(config.h_threshold, done_loc, done_all, config.verbose);
  }

  auto toc = std::chrono::high_resolution_clock::now();
  double eta_secs = std::chrono::duration_cast<std::chrono::duration<double>>(toc - tic).count();

  Console::printf(Console::Color::BRIGHT_GREEN, "\nRank[%d]: 🎉 %d iterations, cost_ub=%.2f, eta=%.2f secs\n", rank, niters, cntrs.cost_ub, eta_secs);

  // Write stats to json
  to_json(eta_secs);

  // Close output file
  Console::close();
}