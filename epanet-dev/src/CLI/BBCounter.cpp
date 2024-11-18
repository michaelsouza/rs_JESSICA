// src/CLI/BBCounter.cpp
#include "BBCounter.h"
#include <algorithm>
#include <iostream>
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
}

bool BBCounter::update_y(bool is_feasible)
{
  if (this->h < 0 || this->h > this->h_max)
  {
    throw std::out_of_range("ERR: h (" + std::to_string(this->h) + ") is out of range in update_y");
  }

  if (this->h == 0 && !is_feasible)
    return false;

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
  if (!verbose)
    return;
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
        if (actuations_csum[pump] >= max_actuations)
          return false;
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
      if (x_new[j] > x_old[j])
        ++actuations_csum[j];
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
    if (!updated)
      return false;
    this->show_xy(true);
  }

  return true;
}
