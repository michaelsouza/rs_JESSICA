// src/CLI/BBStats.h
#pragma once

#include "ColorStream.h"
#include "Utils.h"
#include <map>
#include <string>
#include <vector>

class BBStats {
public:
  BBStats(int h_max, int max_actuations);

  void record_pruning(const std::string &reason, int h);
  void record_feasible(int h);
  void record_solution(double cost, const std::vector<int> &y);

  double cost_min;
  std::vector<int> y_min;
  std::vector<std::map<std::string, int>> prunings;
  std::vector<int> feasible_counter;
  std::vector<std::string> prune_keys;
};
