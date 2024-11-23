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

  void add_pruning(PruneReason reason, int h);
  void add_feasible(int h);

  double cost_min;
  std::vector<int> y_min;
  std::vector<int> x_min;
  std::vector<std::map<PruneReason, int>> prunings;
  std::vector<int> feasible_counter;
  int split_counter;
  std::vector<PruneReason> prune_keys;
  void show() const;
  void to_json(double eta_secs);
};
