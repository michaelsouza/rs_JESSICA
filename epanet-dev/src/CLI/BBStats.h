// src/CLI/BBStats.h
#pragma once

#include "BBPruneReason.h"
#include "BBConfig.h"
#include "BBConstraints.h"
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

  std::vector<std::map<PruneReason, int>> prunings;
  std::vector<int> feasible_counter;
  int split_counter;  
  void to_json(const BBConfig &config, const BBConstraints& cnstr, double eta_secs, std::vector<int>& y_best, std::vector<int>& x_best);
};
