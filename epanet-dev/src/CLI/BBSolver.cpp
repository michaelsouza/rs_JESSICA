// src/CLI/BBCounter.cpp
#include "BBSolver.h"
#include "Console.h"
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <mpi.h>
#include <numeric>
#include <stdexcept>

BBSolver::BBSolver(std::string inpFile, int h_max, int max_actuations) : inpFile(inpFile), cntrs(inpFile), stats(h_max, max_actuations)
{
  // Initialize scalar variables
  this->h_max = h_max;
  this->max_actuations = max_actuations;
  this->num_pumps = cntrs.get_num_pumps();
  this->h = 0;
  this->top_cut = 0;
  this->top_level = 0;
  this->is_feasible = true;

  // Initialize y and x vectors
  this->y = std::vector<int>(h_max + 1, 0);
  this->x = std::vector<int>(num_pumps * (h_max + 1), 0);

  // Allocate the recv buffer
  this->mpi_buffer.resize(3 + y.size() + x.size());
  MPI_Comm_rank(MPI_COMM_WORLD, &this->mpi_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &this->mpi_size);
}

bool BBSolver::update_y()
{
  if (h < 0 || h > h_max)
  {
    throw std::out_of_range("ERR: h (" + std::to_string(h) + ") is out of range in update_y");
  }

  // There is no additional work to do if h == 0 and the current level is infeasible
  if (h == 0 && !is_feasible) return false;

  // If the current level is feasible, increment the time period
  if (is_feasible && h < h_max)
  {
    h++;
    y[h] = 0;
    return true;
  }

  // The level is finished, reset it and go back one level
  if (y[h] == num_pumps)
  {
    // Reset the current level
    y[h] = 0;

    // Go back one level
    h--;

    // Mark the current level as infeasible
    is_feasible = false;

    // Try to update the y vector again
    return update_y();
  }

  // Increment the current level
  if (y[h] < num_pumps)
  {
    y[h]++;
    return true;
  }

  // There is no feasible level
  return false;
}

void BBSolver::jump_to_end()
{
  y[h] = num_pumps;
}

bool BBSolver::update_x(bool verbose)
{
  // Record feasible if the current state is feasible
  if (is_feasible) stats.record_feasible(h);

  // Update x core
  bool is_feasible = this->update_x_core(verbose);

  // Check consistency sum(x[h]) == y[h]
  if (is_feasible)
  {
    const int *x_new = &x[num_pumps * h];
    int sum_x = std::accumulate(x_new, x_new + num_pumps, 0);
    if (sum_x != y[h]) throw std::runtime_error("sum(x)=" + std::to_string(sum_x) + " != y=" + std::to_string(y[h]));
  }

  return is_feasible;
}

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

  if (y_new == y_old)
  {
    return true;
  }

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

int BBSolver::get_free_level()
{
  // Return the top level if its value is lower than the top cut
  if (y[top_level] < top_cut)
  {
    return top_level;
  }

  // Consider only the levels in the range [top_level + 1, h]
  for (int level = top_level + 1; level <= h; level++)
  {
    // For the remaining levels, the cut is the number of pumps
    if (y[level] < num_pumps)
    {
      // Update the top level and top cut
      top_level = level;
      top_cut = num_pumps;

      return top_level;
    }
  }

  // If no free level is found, return the last level
  return h_max;
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
  Console::printf(Console::Color::YELLOW, "   h=%d, is_feasible=%d\n", h, is_feasible);

  // Display Top Level and Top Cut
  Console::printf(Console::Color::BRIGHT_MAGENTA, "   Top: level=%d, cut=%d\n", top_level, top_cut);

  // Display y and x vectors
  show_xy(true);

  // Print Footer
  Console::printf(Console::Color::BRIGHT_CYAN, "\n");

  this->stats.show();

  this->cntrs.show();
}

void BBSolver::write_buffer()
{
  // Retrieve the MPI rank
  int rank = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  // Log the start of buffer writing
  Console::printf(Console::Color::CYAN, "Rank[%d]: writing buffer.\n", rank);

  // Resize the buffer
  const size_t num_scalars = 3;
  mpi_buffer.resize(num_scalars + y.size() + x.size());

  // Write scalar values
  mpi_buffer[0] = top_level;
  mpi_buffer[1] = y[top_level]; // send the current top level value as top_cut
  mpi_buffer[2] = h;

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

  // Log the start of buffer reading
  Console::printf(Console::Color::CYAN, "Rank[%d]: reading buffer.\n", rank);

  // Read scalar values
  const size_t num_scalars = 3;
  top_level = mpi_buffer[0];
  top_cut = mpi_buffer[1];
  h = mpi_buffer[2];

  // Read y vector
  std::copy(mpi_buffer.begin() + num_scalars, mpi_buffer.begin() + num_scalars + y.size(), y.begin());

  // Read x vector
  std::copy(mpi_buffer.begin() + num_scalars + y.size(), mpi_buffer.begin() + num_scalars + y.size() + x.size(), x.begin());
}

void BBSolver::prune(PruneReason reason)
{
  this->stats.record_pruning(reason, this->h);
}

// Function to process a node in the branch-and-bound tree
bool BBSolver::process_node(double &cost, bool verbose, bool save_project)
{
  bool is_feasible = true;
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

    if (verbose) printf("\nSimulation: t_max=%d, t=%d, dt=%d\n", t_max, t, dt);

    // Check node pressures
    is_feasible = cntrs.check_pressures(verbose);
    if (!is_feasible)
    {
      prune(PruneReason::PRESSURES);
      break;
    }

    // Check tank levels
    is_feasible = cntrs.check_levels(verbose);
    if (!is_feasible)
    {
      prune(PruneReason::LEVELS);
      break;
    }

    // Check cost
    cost = cntrs.calc_cost();
    is_feasible = cntrs.check_cost(cost, verbose);
    if (!is_feasible)
    {
      prune(PruneReason::COST);
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
    if (!is_feasible) prune(PruneReason::STABILITY);
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

void BBSolver::update_pumps(EN_Project p, bool verbose)
{
  cntrs.update_pumps(p, this->h, this->x, verbose);
}

void BBSolver::set_feasible()
{
  is_feasible = true;
  stats.record_feasible(h);
}

void BBSolver::send_work(int recv_rank, const std::vector<int> &free_level, bool verbose)
{
  if (verbose) Console::printf(Console::Color::BRIGHT_MAGENTA, "Rank[%d]: Sending to rank %d\n", mpi_rank, recv_rank);
  write_buffer();
  auto mpi_error = MPI_Send(mpi_buffer.data(), mpi_buffer.size(), MPI_INT, recv_rank, 0, MPI_COMM_WORLD);
  if (mpi_error != MPI_SUCCESS)
  {
    if (verbose) Console::printf(Console::Color::RED, "Rank[%d]: MPI_Send failed with error code %d.\n", mpi_rank, mpi_error);
    MPI_Abort(MPI_COMM_WORLD, mpi_error);
  }

  // Update status
  h = top_level;
  is_feasible = false;
  prune(PruneReason::SPLIT);

  show();
}

void BBSolver::recv_work(int send_rank, const std::vector<int> &free_level, bool verbose)
{
  if (verbose) Console::printf(Console::Color::BRIGHT_MAGENTA, "Rank[%d]: Receiving from rank %d\n", mpi_rank, send_rank);
  auto mpi_error = MPI_Recv(mpi_buffer.data(), mpi_buffer.size(), MPI_INT, send_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  if (mpi_error != MPI_SUCCESS)
  {
    if (verbose) Console::printf(Console::Color::RED, "Rank[%d]: MPI_Recv failed with error code %d.\n", mpi_rank, mpi_error);
    MPI_Abort(MPI_COMM_WORLD, mpi_error);
  }
  read_buffer();
  show();
}

void BBSolver::split(const std::vector<int> &done, const std::vector<int> &free_level, int level_max, bool verbose)
{
  // Count the number of sending ranks
  int count_send = 0, count_recv = 0;
  for (int send_rank = 0; send_rank < mpi_size; ++send_rank)
  {
    // Skip if the rank is done, it has nothing to send
    if (done[send_rank]) continue;

    // Skip if the rank work is too small (high level)
    if (free_level[send_rank] > level_max) continue;

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
      if (send_rank == mpi_rank) send_work(recv_rank, free_level, verbose);

      // This rank is a receiver
      if (recv_rank == mpi_rank) recv_work(send_rank, free_level, verbose);
    }
  }
}
