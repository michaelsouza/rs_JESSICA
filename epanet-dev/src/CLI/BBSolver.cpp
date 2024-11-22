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

BBSolver::BBSolver(std::string inpFile, int h_max, int max_actuations) : inpFile(inpFile), cntrs(inpFile), stats(h_max, max_actuations)
{
  // Initialize scalar variables
  this->h_max = h_max;
  this->max_actuations = max_actuations;
  this->num_pumps = cntrs.get_num_pumps();
  this->h = 0;
  this->h_cut = 0;
  this->h_min = 0;
  this->is_feasible = true;

  // Initialize y and x vectors
  this->y = std::vector<int>(h_max + 1, 0);
  this->x = std::vector<int>(num_pumps * (h_max + 1), 0);
  this->y_best = std::vector<int>(h_max + 1, 0);
  this->x_best = std::vector<int>(num_pumps * (h_max + 1), 0);

  // Allocate the recv buffer
  this->mpi_buffer.resize(4 + y.size() + x.size());
  MPI_Comm_rank(MPI_COMM_WORLD, &this->mpi_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &this->mpi_size);
}

/** Main functions */
bool BBSolver::process_node(double &cost, bool verbose, bool save_project)
{
  is_feasible = true;
  int t = 0, dt = 0, t_max = 3600 * this->h;

  EN_Project p = EN_createProject();

  CHK(EN_loadProject(this->inpFile.c_str(), p), "Load project");
  CHK(EN_initSolver(EN_INITFLOW, p), "Initialize solver");

  // Set the project and constraints
  update_pumps(p, verbose);

  if (verbose) show();

  do
  {
    // Run the solver
    CHK(EN_runSolver(&t, p), "Run solver");

    if (verbose) Console::printf(Console::Color::MAGENTA, "\nSimulation: t_max=%d, t=%d, dt=%d\n", t_max, t, dt);

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

    // Check cost
    cost = cntrs.calc_cost();
    is_feasible = cntrs.check_cost(cost, verbose);
    if (!is_feasible)
    {
      add_prune(PruneReason::COST);
      jump_to_end();
      break;
    }

    // Advance the solver
    CHK(EN_advanceSolver(&dt, p), "Advance solver");

    // Check if we have reached the maximum simulation time
    if (t + dt > t_max) break;
  } while (dt > 0);

  // Check stability for the last hour
  if (is_feasible && this->h == this->h_max)
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
    Project *prj = static_cast<Project *>(p);
    prj->save(ss.str().c_str());
    Console::printf(Console::Color::BRIGHT_GREEN, "Project saved to: %s\n", ss.str().c_str());
  }

  // Delete the project
  EN_deleteProject(p);

  return is_feasible;
}

/** Update functions */
void BBSolver::update_pumps(EN_Project p, bool verbose)
{
  cntrs.update_pumps(p, this->h, this->x, verbose);
}

bool BBSolver::set_y(const std::vector<int> &y)
{
  // Copy y to the internal y vector
  this->y = y;

  // Start assuming the y vector is feasible
  this->is_feasible = true;

  // Set y and update x
  this->h = 0;
  for (int i = 0; i < this->h_max; ++i)
  {
    this->h++;
    bool updated = this->update_x(false);
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
  // Record feasible if the current state is feasible
  if (is_feasible) stats.add_feasible(h);

  // Update x core
  is_feasible = this->update_x_core(verbose);

  // Check consistency sum(x[h]) == y[h]
  if (is_feasible)
  {
    const int *x_new = &x[num_pumps * h];
    int sum_x = std::accumulate(x_new, x_new + num_pumps, 0);
    if (sum_x != y[h]) throw std::runtime_error("sum(x)=" + std::to_string(sum_x) + " != y=" + std::to_string(y[h]));
  }

  return is_feasible;
}

bool BBSolver::update_x_core(bool verbose)
{
  // Get handle variables
  const int h = this->h;
  const int max_actuations = this->max_actuations;
  const std::vector<int> &y = this->y;
  std::vector<int> &x = this->x;

  // Get the previous and new states
  const int &y_old = y[h - 1];
  const int &y_new = y[h];
  const int *x_old = &x[this->num_pumps * (h - 1)];
  int *x_new = &x[this->num_pumps * h];

  // Start by copying the previous state
  std::copy(x_old, x_old + this->num_pumps, x_new);

  if (y_new == y_old) return true;

  // Calculate the cumulative actuations up to hour h
  int actuations_csum[this->num_pumps];
  this->calc_actuations_csum(actuations_csum, x, h);

  // Get the sorted indices of the actuations_csum
  int pumps_sorted[this->num_pumps];
  std::iota(pumps_sorted, pumps_sorted + this->num_pumps, 0);
  std::sort(pumps_sorted, pumps_sorted + this->num_pumps, [&](int i, int j) { return actuations_csum[i] < actuations_csum[j]; });

  if (y_new > y_old)
  {
    int num_actuations = y_new - y_old;
    // Identify pumps that are not currently actuating
    for (int i = 0; i < this->num_pumps && num_actuations > 0; i++)
    {
      const int pump = pumps_sorted[i];
      if (x_new[pump] == 0)
      {
        if (actuations_csum[pump] >= max_actuations) return false;
        x_new[pump] = 1;
        --num_actuations;
      }
    }
    return num_actuations == 0;
  }

  if (y_new < y_old)
  {
    int num_deactuations = y_old - y_new;
    for (int i = 0; i < this->num_pumps && num_deactuations > 0; i++)
    {
      const int pump = pumps_sorted[i];
      if (x_new[pump] == 1)
      {
        x_new[pump] = 0;
        --num_deactuations;
      }
    }
    return num_deactuations == 0;
  }

  return true;
}

void BBSolver::jump_to_end()
{
  if (h == h_min)
    y[h] = h_cut;
  else
    y[h] = num_pumps;
}

void BBSolver::calc_actuations_csum(int *actuations_csum, const std::vector<int> &x, int h)
{
  // Set actuations_csum to 0
  std::fill(actuations_csum, actuations_csum + this->num_pumps, 0);

  // Calculate the cumulative actuations up to hour h
  for (int i = 2; i < h; i++)
  {
    const int *x_old = &x[this->num_pumps * (i - 1)];
    const int *x_new = &x[this->num_pumps * i];
    for (int j = 0; j < this->num_pumps; j++)
    {
      if (x_new[j] > x_old[j]) ++actuations_csum[j];
    }
  }
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

void BBSolver::show() const
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

  // Print Footer
  Console::printf(Console::Color::BRIGHT_CYAN, "\n");

  this->stats.show();

  this->cntrs.show();
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

void BBSolver::update_cost(double cost, bool update_xy)
{
  if (cost > cntrs.cost_ub)
  {
    Console::printf(Console::Color::RED, "ERR[rank=%d]: cost=%.2f > cost_max=%.2f\n", mpi_rank, cost, cntrs.cost_ub);
    MPI_Abort(MPI_COMM_WORLD, 1);
  }

  std::string cost_ub_str = (cntrs.cost_ub > 999999999) ? "inf" : std::to_string(cntrs.cost_ub);
  Console::printf(Console::Color::GREEN, "\nRank[%d]: 💰 updated cost_ub=%.2f (%s)\n", mpi_rank, cost, cost_ub_str.c_str());

  cntrs.cost_ub = cost;

  if (update_xy)
  {
    y_best = y;
    x_best = x;
  }
}

inline void solve_iteration(BBSolver &solver, int &done_loc, bool verbose, bool save_project)
{
  if (done_loc) return;

  double cost = 0.0;

  // Update y vector
  done_loc = !solver.update_y();
  if (done_loc)
  {
    Console::printf(Console::Color::BRIGHT_RED, "\nRank[%d]: done_loc=true\n", solver.mpi_rank);
    return;
  }

  // Update x vector
  solver.update_x(verbose);
  if (!solver.is_feasible)
  {
    solver.add_prune(PruneReason::ACTUATIONS);
    return;
  }

  // Process node
  solver.process_node(cost, false, save_project);
  // std::this_thread::sleep_for(std::chrono::milliseconds(300)); //TODO: remove

  // Update feasible counter
  if (solver.is_feasible)
  {
    solver.add_feasible();

    // New solution found
    if (solver.h == solver.h_max)
    {
      solver.update_cost(cost, true);
    }
  }
}

inline void solve_sync(const int h_threshold, BBSolver &solver, int &done_loc, int &done_all, bool verbose)
{
  static int num_calls = 0;
  num_calls++;
  Console::hline(Console::Color::BRIGHT_CYAN);
  Console::printf(Console::Color::BRIGHT_CYAN, "Rank[%d]: solve_sync #%d (done_loc=%d)\n", solver.mpi_rank, num_calls, done_loc);

  int mpi_error;
  std::vector<int> done(solver.mpi_size, 0);
  std::vector<int> h_free(solver.mpi_size, 0);
  std::vector<double> cost_ub(solver.mpi_size, 0.0);

  // Synchronize cost_best across all ranks ================================
  mpi_error = MPI_Allgather(&solver.cntrs.cost_ub, 1, MPI_DOUBLE, cost_ub.data(), 1, MPI_DOUBLE, MPI_COMM_WORLD);
  if (mpi_error != MPI_SUCCESS) MPI_Abort(MPI_COMM_WORLD, mpi_error);

  double cost_min = std::numeric_limits<double>::infinity();
  int rank_min = 0;
  for (int rank = 0; rank < solver.mpi_size; ++rank)
  {
    if (cost_ub[rank] < cost_min)
    {
      cost_min = cost_ub[rank];
      rank_min = rank;
    }
  }

  if (solver.cntrs.cost_ub > cost_min)
  {
    solver.update_cost(cost_min, false);
  }

  // Broadcast the solution from the rank with the minimum cost
  mpi_error = MPI_Bcast(solver.y_best.data(), solver.y_best.size(), MPI_INT, rank_min, MPI_COMM_WORLD);
  if (mpi_error != MPI_SUCCESS)
  {
    Console::printf(Console::Color::RED, "Rank[%d]: MPI_Bcast failed with error code %d.\n", solver.mpi_rank, mpi_error);
    MPI_Abort(MPI_COMM_WORLD, mpi_error);
  }
  mpi_error = MPI_Bcast(solver.x_best.data(), solver.x_best.size(), MPI_INT, rank_min, MPI_COMM_WORLD);
  if (mpi_error != MPI_SUCCESS)
  {
    Console::printf(Console::Color::RED, "Rank[%d]: MPI_Bcast failed with error code %d.\n", solver.mpi_rank, mpi_error);
    MPI_Abort(MPI_COMM_WORLD, mpi_error);
  }

  // Synchronize done_all across all ranks =================================
  mpi_error = MPI_Allgather(&done_loc, 1, MPI_INT, done.data(), 1, MPI_INT, MPI_COMM_WORLD);
  if (mpi_error != MPI_SUCCESS) MPI_Abort(MPI_COMM_WORLD, mpi_error);

  // Check if all ranks are done, if so, break
  done_all = std::all_of(done.begin(), done.end(), [](int i) { return i == 1; });
  if (done_all) return;

  // Update h_free with data from all ranks ================================
  int h_free_loc = solver.get_free_level();
  mpi_error = MPI_Allgather(&h_free_loc, 1, MPI_INT, h_free.data(), 1, MPI_INT, MPI_COMM_WORLD);
  if (mpi_error != MPI_SUCCESS) MPI_Abort(MPI_COMM_WORLD, mpi_error);

  // The work is not done if the task has been split
  bool done_split = solver.try_split(done, h_free, h_threshold, verbose);
  if (done_loc) done_loc = !done_split;

  Console::printf(Console::Color::BRIGHT_MAGENTA, "Rank[%d]: MPI_Barrier (h=%d, done_split=%d, done_loc=%d)\n", solver.mpi_rank, solver.h, done_split,
                  done_loc);
  MPI_Barrier(MPI_COMM_WORLD);
}

void solve(int argc, char *argv[])
{
  // Get MPI rank and size
  int rank = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  // Default input file
  const char *inpFile = "/home/michael/gitrepos/rs_JESSICA/networks/any-town.inp";

  // Parse command line arguments
  int h_max = 24;
  int max_actuations = 3;
  bool verbose = false;
  bool save_project = false;
  bool use_logger = false;
  int h_threshold = 18;
  parse_args(argc, argv, verbose, h_max, max_actuations, save_project, use_logger, h_threshold);

  // Open logger file for each rank
  Console::open(rank, use_logger, verbose);

  // Initialize solver
  BBSolver solver(inpFile, h_max, max_actuations);

  // Initialize iteration variables
  int done_loc = (rank != 0); // Only rank 0 starts
  int done_all = 0;

  // Main loop
  static int niters = 0;
  while (!done_all)
  {
    ++niters;
    solve_iteration(solver, done_loc, verbose, save_project);
    solve_sync(h_threshold, solver, done_loc, done_all, verbose);
  }

  Console::printf(Console::Color::BRIGHT_GREEN, "Rank[%d]: 🎉 %d iterations, cost_ub=%.2f\n", rank, niters, solver.cntrs.cost_ub);

  // Close output file
  Console::close();
}
