// src/CLI/BBCounter.cpp
#include "BBSolver.h"
#include "ColorStream.h"
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <mpi.h>
#include <numeric>
#include <stdexcept>

BBSolver::BBSolver(std::string inpFile, int h_max, int max_actuations)
    : inpFile(inpFile), cntrs(inpFile), stats(h_max, max_actuations)
{
  // Initialize scalar variables
  this->h_max = h_max;
  this->max_actuations = max_actuations;
  this->num_pumps = cntrs.get_num_pumps();
  this->h = 0;
  this->top_cut = 0;
  this->top_level = 0;

  // Initialize y and x vectors
  this->y = std::vector<int>(h_max + 1, 0);
  this->x = std::vector<int>(num_pumps * (h_max + 1), 0);

  // Calculate the buffer size
  this->buffer_size = this->y.size() + this->x.size() + 6;
}

bool BBSolver::update_y()
{
  bool is_feasible = this->is_feasible;
  if (this->h < 0 || this->h > this->h_max)
  {
    throw std::out_of_range("ERR: h (" + std::to_string(this->h) + ") is out of range in update_y");
  }

  if (this->h == 0 && !is_feasible) return false;

  if (is_feasible && this->h < this->h_max)
  {
    this->h++;
    this->y[this->h] = 0;
    return true;
  }

  // If the current state is at the maximum actuations, reset it and go back one level
  if (this->y[this->h] == this->y_max)
  {
    this->y[this->h] = 0;
    this->h--;
    this->is_feasible = false;
    return this->update_y();
  }

  if (this->y[this->h] < this->y_max)
  {
    this->y[this->h]++;
    return true;
  }

  // There is no feasible state
  return false;
}

void BBSolver::jump_to_end()
{
  this->y[this->h] = this->y_max;
}

bool BBSolver::update_x(bool verbose)
{
  // Record feasible if the current state is feasible
  if (this->is_feasible) this->stats.record_feasible(this->h);

  // Update x core
  bool is_feasible = this->update_x_core(verbose);

  // Check consistency sum(x[h]) == y[h]
  if (is_feasible)
  {
    const int *x_new = &this->x[this->num_pumps * this->h];
    int sum_x = std::accumulate(x_new, x_new + this->num_pumps, 0);
    if (sum_x != this->y[this->h])
      throw std::runtime_error("sum(x)=" + std::to_string(sum_x) + " != y=" + std::to_string(this->y[this->h]));
  }

  return is_feasible;
}

void BBSolver::show_xy(bool verbose)
{
  if (!verbose) return;
  std::cout << "\n";
  for (int i = 1; i <= this->h; i++)
  {
    printf("h[%2d]: y=%d, x=[", i, this->y[i]);
    const int *x_i = &this->x[this->num_pumps * i];
    for (int j = 0; j < this->num_pumps; j++)
    {
      printf("%d ", x_i[j]);
    }
    std::cout << "]" << std::endl;
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
  std::sort(pumps_sorted, pumps_sorted + this->num_pumps,
            [&](int i, int j) { return actuations_csum[i] < actuations_csum[j]; });

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
  if (this->y[this->top_level] < this->top_cut)
  {
    return this->top_level;
  }

  // Consider only the levels in the range [top_level + 1, h]
  for (int level = this->top_level + 1; level <= this->h; level++)
  {
    if (this->y[level] < this->max_actuations) return level;
  }
  return this->h_max;
}

void BBSolver::show() const
{
  // Get MPI rank
  int rank = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  // Print Header with MPI rank
  ColorStream::printf(ColorStream::Color::BRIGHT_CYAN, "=== BBCounter Current State (Rank %d) ===", rank);

  // Display Current Time Period
  ColorStream::printf(ColorStream::Color::YELLOW, "Current Time Period (h): ");
  std::cout << this->h << std::endl;

  // Display y vector up to current h
  ColorStream::printf(ColorStream::Color::BRIGHT_BLUE, "Actuations (y):");
  for (int i = 0; i <= this->h && i <= this->h_max; ++i)
  {
    std::cout << "  h[" << std::setw(2) << i << "]: y = " << this->y[i] << std::endl;
  }

  // Display x vector for current h
  ColorStream::printf(ColorStream::Color::BRIGHT_BLUE, "Pump States (x) at Current Time Period:");
  const int *x_current = &this->x[this->num_pumps * this->h];
  for (int j = 0; j < this->num_pumps; ++j)
  {
    ColorStream::printf(ColorStream::Color::YELLOW, "  Pump %d: ", j + 1);
    if (x_current[j] == 1)
    {
      ColorStream::printf(ColorStream::Color::GREEN, "Active");
    }
    else
    {
      ColorStream::printf(ColorStream::Color::RED, "Inactive");
    }
  }

  // Display Top Level and Top Cut
  ColorStream::printf(ColorStream::Color::BRIGHT_MAGENTA, "Top Level: ");
  std::cout << this->top_level << std::endl;
  ColorStream::printf(ColorStream::Color::BRIGHT_MAGENTA, "Top Cut: ");
  std::cout << this->top_cut << std::endl;

  // Optional: Display Additional Metrics or Information
  // For example, cumulative actuations, remaining actuations, etc.
  // ...

  // Print Footer
  ColorStream::printf(ColorStream::Color::BRIGHT_CYAN, "================================");

  this->stats.summary();
}

void BBSolver::write_buffer(std::vector<int> &recv_buffer) const
{
  // Retrieve the MPI rank
  int rank = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  // Log the start of buffer writing
  ColorStream::printf(ColorStream::Color::CYAN, "Rank[%d]: writing buffer.", rank);

  // Resize the buffer
  recv_buffer.resize(this->buffer_size);

  // Write scalar values
  recv_buffer[0] = top_level;
  recv_buffer[1] = top_cut;
  recv_buffer[2] = h;
  recv_buffer[3] = y_max;
  recv_buffer[4] = h_max;
  recv_buffer[5] = max_actuations;

  // Write y vector
  std::copy(y.begin(), y.end(), recv_buffer.begin() + 6);

  // Write x vector
  std::copy(x.begin(), x.end(), recv_buffer.begin() + 6 + y.size());
}

void BBSolver::read_buffer(const std::vector<int> &recv_buffer)
{
  // Retrieve the MPI rank
  int rank = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  // Log the start of buffer reading
  ColorStream::printf(ColorStream::Color::CYAN, "Rank[%d]: reading buffer.", rank);

  // Read scalar values
  top_level = recv_buffer[0];
  top_cut = recv_buffer[1];
  h = recv_buffer[2];
  y_max = recv_buffer[3];
  h_max = recv_buffer[4];
  max_actuations = recv_buffer[5];

  // Read y vector
  y.resize(h_max + 1);
  std::copy(recv_buffer.begin() + 6, recv_buffer.begin() + 6 + y.size(), y.begin());

  // Read x vector
  x.resize(num_pumps * (h_max + 1));
  std::copy(recv_buffer.begin() + 6 + y.size(), recv_buffer.begin() + 6 + y.size() + x.size(), x.begin());
}

void BBSolver::split(std::vector<int> &recv_buffer)
{
  write_buffer(recv_buffer);
  const int free_level = get_free_level();
  const int free_level_cut = this->y[free_level];

  // Update the buffer with the free level and free level cut
  recv_buffer[0] = free_level;
  recv_buffer[1] = free_level_cut;

  // Update this counter with last state of the free level and mark it as infeasible
  this->h = free_level + 1;
  this->y[this->h] = h_max;
  this->prune(PruneReason::SPLIT);
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
  Project *prj = static_cast<Project *>(p);

  CHK(EN_loadProject(this->inpFile.c_str(), p), "Load project");
  CHK(EN_initSolver(EN_INITFLOW, p), "Initialize solver");

  // Set the project and constraints
  set_project(p, verbose);

  do
  {
    // Run the solver
    CHK(EN_runSolver(&t, p), "Run solver");

    if (verbose) printf("\nSimulation: t_max=%d, t: %d, dt: %d\n", t_max, t, dt);

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
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << "output_" << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S") << ".inp";
    prj->save(ss.str().c_str());
    ColorStream::printf(ColorStream::Color::BRIGHT_GREEN, "Project saved to: %s\n", ss.str().c_str());
  }

  // Delete the project
  EN_deleteProject(p);

  return is_feasible;
}

void BBSolver::set_project(EN_Project p, bool verbose)
{
  cntrs.set_project(p, this->h, this->x, verbose);
}