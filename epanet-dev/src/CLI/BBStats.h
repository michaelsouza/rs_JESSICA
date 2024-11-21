// src/CLI/BBStats.h
#pragma once

#include "BBPruneReason.h"
#include "Console.h"
#include "Utils.h"

#include <map>
#include <string>
#include <vector>

class BBStats
{
public:
  BBStats(int h_max, int max_actuations);

  void record_pruning(PruneReason reason, int h);
  void record_feasible(int h);
  void record_solution(double cost, const std::vector<int> &y);

  double cost_min;
  std::vector<int> y_min;
  std::vector<std::map<PruneReason, int>> prunings;
  std::vector<int> feasible_counter;
  std::vector<PruneReason> prune_keys;
  void show() const;
};
