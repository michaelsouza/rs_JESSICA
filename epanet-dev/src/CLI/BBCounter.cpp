// src/CLI/BBCounter.cpp
#include "BBCounter.h"
#include "ColorStream.h"
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <mpi.h>
#include <numeric>
#include <stdexcept>

BBCounter::BBCounter(int y_max, int h_max, int max_actuations, int num_pumps)
{
  this->y_max = y_max;
  this->h_max = h_max;
  this->max_actuations = max_actuations;
  this->num_pumps = num_pumps;
  this->y = std::vector<int>(h_max + 1, 0);
  this->x = std::vector<int>(num_pumps * (h_max + 1), 0);
  this->h = 0;
  this->top_cut = 0;
  this->top_level = 0;
}

bool BBCounter::update_y(bool is_feasible)
{
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

  if (this->y[this->h] == this->y_max)
  {
    this->y[this->h] = 0;
    this->h--;
    return this->update_y(false);
  }

  if (this->y[this->h] < this->y_max)
  {
    this->y[this->h]++;
    return true;
  }

  return false;
}

void BBCounter::jump_to_end()
{
  this->y[this->h] = this->y_max;
}

bool BBCounter::update_x(bool verbose)
{
  bool is_feasible = this->update_x_core(verbose);
  if (is_feasible)
  {
    // Check consistency
    const int *x_new = &this->x[this->num_pumps * this->h];
    int sum_x = std::accumulate(x_new, x_new + this->num_pumps, 0);
    if (sum_x != this->y[this->h])
      throw std::runtime_error("sum(x)=" + std::to_string(sum_x) + " != y=" + std::to_string(this->y[this->h]));
  }
  return is_feasible;
}

void BBCounter::show_xy(bool verbose)
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

bool BBCounter::update_x_core(bool verbose)
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

void BBCounter::calc_actuations_csum(int *actuations_csum, const std::vector<int> &x, int h)
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

bool BBCounter::set_y(const std::vector<int> &y)
{
  // Copy y to the internal y vector
  this->y = y;

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

int BBCounter::top_level_free()
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

void BBCounter::show() const
{
  // Get MPI rank
  int rank = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  // Print Header with MPI rank
  ColorStream::println("=== BBCounter Current State (Rank " + std::to_string(rank) + ") ===",
                       ColorStream::Color::BRIGHT_CYAN);

  // Display Current Time Period
  std::string time_period_label = "Current Time Period (h): ";
  ColorStream::print(time_period_label, ColorStream::Color::YELLOW);
  std::cout << this->h << std::endl;

  // Display y vector up to current h
  ColorStream::println("Actuations (y):", ColorStream::Color::BRIGHT_BLUE);
  for (int i = 0; i <= this->h && i <= this->h_max; ++i)
  {
    std::cout << "  h[" << std::setw(2) << i << "]: y = " << this->y[i] << std::endl;
  }

  // Display x vector for current h
  ColorStream::println("Pump States (x) at Current Time Period:", ColorStream::Color::BRIGHT_BLUE);
  const int *x_current = &this->x[this->num_pumps * this->h];
  for (int j = 0; j < this->num_pumps; ++j)
  {
    std::string pump_label = "  Pump " + std::to_string(j + 1) + ": ";
    ColorStream::print(pump_label, ColorStream::Color::YELLOW);
    if (x_current[j] == 1)
    {
      ColorStream::println("Active", ColorStream::Color::GREEN);
    }
    else
    {
      ColorStream::println("Inactive", ColorStream::Color::RED);
    }
  }

  // Display Top Level and Top Cut
  ColorStream::println("Top Level: " + std::to_string(this->top_level), ColorStream::Color::BRIGHT_MAGENTA);
  ColorStream::println("Top Cut: " + std::to_string(this->top_cut), ColorStream::Color::BRIGHT_MAGENTA);

  // Optional: Display Additional Metrics or Information
  // For example, cumulative actuations, remaining actuations, etc.
  // ...

  // Print Footer
  ColorStream::println("================================", ColorStream::Color::BRIGHT_CYAN);
}

void BBCounter::write_buffer(std::vector<int> &recv_buffer) const
{
  // Retrieve the MPI rank
  int rank = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  // Log the start of buffer writing
  ColorStream::println("Rank[" + std::to_string(rank) + "]: writing buffer.", ColorStream::Color::CYAN);

  // Calculate required buffer size: scalar values + y vector + x vector
  size_t buffer_size = 6 + y.size() + x.size();
  recv_buffer.resize(buffer_size);

  // Write scalar values
  recv_buffer[0] = h;
  recv_buffer[1] = y_max;
  recv_buffer[2] = h_max;
  recv_buffer[3] = max_actuations;
  recv_buffer[4] = top_level;
  recv_buffer[5] = top_cut;

  // Write y vector
  std::copy(y.begin(), y.end(), recv_buffer.begin() + 6);

  // Write x vector
  std::copy(x.begin(), x.end(), recv_buffer.begin() + 6 + y.size());
}

void BBCounter::read_buffer(const std::vector<int> &recv_buffer)
{
  // Retrieve the MPI rank
  int rank = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  // Log the start of buffer reading
  ColorStream::println("Rank[" + std::to_string(rank) + "]: reading buffer.", ColorStream::Color::CYAN);

  // Read scalar values
  h = recv_buffer[0];
  y_max = recv_buffer[1];
  h_max = recv_buffer[2];
  max_actuations = recv_buffer[3];
  top_level = recv_buffer[4];
  top_cut = recv_buffer[5];

  // Read y vector
  y.resize(h_max + 1);
  std::copy(recv_buffer.begin() + 6, recv_buffer.begin() + 6 + y.size(), y.begin());

  // Read x vector
  x.resize(num_pumps * (h_max + 1));
  std::copy(recv_buffer.begin() + 6 + y.size(), recv_buffer.begin() + 6 + y.size() + x.size(), x.begin());
}