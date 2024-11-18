// src/CLI/BBCounter.h
#pragma once

#include <map>
#include <string>
#include <vector>

class BBCounter {
public:
  BBCounter(int y_max, int h_max, int max_actuations, int num_pumps);

  bool update_y(bool is_feasible);
  bool update_x(bool verbose = false);
  void show_xy(bool verbose = false);
  void jump_to_end();

  int h;
  int y_max;
  int h_max;
  std::vector<int> y;
  std::vector<int> x;
  int max_actuations;
  int num_pumps;

private:
  bool update_x_core(bool verbose = false);
  void calc_actuations_csum(int *actuations_csum, const std::vector<int> &x,
                            int h);
};
