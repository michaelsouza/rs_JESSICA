// src/CLI/BBStats.cpp
#include "BBStats.h"
#include "Utils.h" // For ColorStream
#include <iostream>
#include <limits>

BBStats::BBStats(int h_max, int max_actuations)
{
  this->cost_min = std::numeric_limits<double>::infinity();
  this->y_min = std::vector<int>(h_max + 1, 0);
  this->prunings = std::vector<std::map<std::string, int>>(h_max + 1);
  this->feasible_counter = std::vector<int>(h_max + 1, 0);

  // Prune Keys: actuations, cost, pressures, levels, stability
  this->prune_keys = {"actuations", "cost", "pressures", "levels", "stability"};

  for (int h = 0; h <= h_max; h++)
  {
    this->feasible_counter[h] = 0;
    this->prunings[h] = std::map<std::string, int>();
    for (const std::string &key : this->prune_keys)
      this->prunings[h][key] = 0;
  }
}

void BBStats::record_pruning(const std::string &reason, int h)
{
  this->prunings[h][reason]++;
}

void BBStats::record_feasible(int h)
{
  this->feasible_counter[h]++;
}

void BBStats::record_solution(double cost, const std::vector<int> &y)
{
  if (cost < this->cost_min)
  {
    this->cost_min = cost;
    this->y_min = y;

    // Print cost in bright green
    ColorStream::println("\ncost_min: " + std::to_string(this->cost_min), ColorStream::Color::BRIGHT_GREEN);

    // Print y_min in bright yellow
    ColorStream::print("y_min: {", ColorStream::Color::BRIGHT_YELLOW);
    for (size_t i = 1; i < this->y_min.size(); i++)
    {
      ColorStream::print(std::to_string(this->y_min[i]) + ", ", ColorStream::Color::BRIGHT_CYAN);
    }
    ColorStream::println("}", ColorStream::Color::BRIGHT_YELLOW);
    this->summary();
  }
}

void BBStats::summary() 
{
    // Header
    ColorStream::println("\nBranch and Bound Statistics", ColorStream::Color::BRIGHT_WHITE);
    ColorStream::println("═══════════════════════════", ColorStream::Color::BRIGHT_WHITE);
    
    // Best solution
    ColorStream::print("Best cost: ", ColorStream::Color::WHITE);
    ColorStream::println(std::to_string(cost_min), ColorStream::Color::BRIGHT_GREEN);
    
    // Table header
    ColorStream::print("Level │ ", ColorStream::Color::BRIGHT_WHITE);
    for (const auto& key : prune_keys) {
        ColorStream::print(key + " │ ", ColorStream::Color::BRIGHT_WHITE);
    }
    ColorStream::println("Feasible", ColorStream::Color::BRIGHT_WHITE);
    
    // Separator line
    ColorStream::print("------+", ColorStream::Color::WHITE);
    for (size_t i = 0; i < prune_keys.size(); i++) {
        std::string separator(prune_keys[i].length() + 2, '-');
        ColorStream::print(separator + "+", ColorStream::Color::WHITE);
    }
    ColorStream::println("---------", ColorStream::Color::WHITE);
    
    // Data rows
    for (size_t h = 0; h < prunings.size(); h++) {
        // Level number
        ColorStream::print(std::to_string(h) + std::string(5 - std::to_string(h).length(), ' ') + " │ ", 
                          ColorStream::Color::YELLOW);
        
        // Pruning counts
        for (const auto& key : prune_keys) {
            std::string value = std::to_string(prunings[h][key]);
            ColorStream::print(value + std::string(key.length() + 2 - value.length(), ' ') + "│ ", 
                             ColorStream::Color::CYAN);
        }
        
        // Feasible solutions
        ColorStream::println(std::to_string(feasible_counter[h]), ColorStream::Color::GREEN);
    }
}
