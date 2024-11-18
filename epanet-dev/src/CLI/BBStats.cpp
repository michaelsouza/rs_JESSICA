// src/CLI/BBStats.cpp
#include "BBStats.h"
#include "Utils.h" // For ColorStream
#include <iostream>
#include <limits>

BBStats::BBStats(int h_max, int max_actuations) {
  this->cost_min = std::numeric_limits<double>::infinity();
  this->y_min = std::vector<int>(h_max + 1, 0);
  this->prunings = std::vector<std::map<std::string, int>>(h_max + 1);
  this->feasible_counter = std::vector<int>(h_max + 1, 0);

  // Prune Keys: actuations, cost, pressures, levels, stability
  this->prune_keys = {"actuations", "cost", "pressures", "levels", "stability"};

  for (int h = 0; h <= h_max; h++) {
    this->feasible_counter[h] = 0;
    this->prunings[h] = std::map<std::string, int>();
    for (const std::string &key : this->prune_keys)
      this->prunings[h][key] = 0;
  }
}

void BBStats::record_pruning(const std::string &reason, int h) {
  this->prunings[h][reason]++;
}

void BBStats::record_feasible(int h) { this->feasible_counter[h]++; }

void BBStats::record_solution(double cost, const std::vector<int> &y) {
  if (cost < this->cost_min) {
    this->cost_min = cost;
    this->y_min = y;

    // Print cost in bright green
    ColorStream::println("\ncost_min: " + std::to_string(this->cost_min),
                         ColorStream::Color::BRIGHT_GREEN);

    // Print y_min in bright yellow
    ColorStream::print("y_min: [", ColorStream::Color::BRIGHT_YELLOW);
    for (size_t i = 1; i < this->y_min.size(); i++) {
      ColorStream::print(std::to_string(this->y_min[i]) + " ",
                         ColorStream::Color::BRIGHT_CYAN);
    }
    ColorStream::println("]", ColorStream::Color::BRIGHT_YELLOW);
  }
}
