// src/CLI/BBStats.cpp
#include "BBStats.h"
#include "Utils.h" // For ColorStream
#include <iostream>
#include <limits>
#include <mpi.h>

BBStats::BBStats(int h_max, int max_actuations)
{
  this->cost_min = std::numeric_limits<double>::infinity();
  this->y_min = std::vector<int>(h_max + 1, 0);
  this->feasible_counter = std::vector<int>(h_max + 1, 0);
  this->prunings = std::vector<std::map<PruneReason, int>>(h_max + 1);
  // Initialize prunings
  std::vector<PruneReason> reasons = {PruneReason::ACTUATIONS, PruneReason::COST, PruneReason::PRESSURES, PruneReason::LEVELS,
                                      PruneReason::STABILITY};
  for (int h = 0; h <= h_max; ++h)
  {
    for (auto &reason : reasons)
    {
      this->prunings[h][reason] = 0;
    }
  }
}

void BBStats::record_pruning(PruneReason reason, int h)
{
  this->prunings[h][reason]++;
}

void BBStats::record_feasible(int h)
{
  this->feasible_counter[h]++;
}

void BBStats::show() const
{
  // Get MPI rank
  int rank = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  // Header with rank info
  Console::hline(Console::Color::BRIGHT_WHITE);
  Console::printf(Console::Color::BRIGHT_WHITE, "Statistics (Rank %d)\n", rank);

  // Best solution
  Console::printf(Console::Color::WHITE, "Best cost: ");
  Console::printf(Console::Color::BRIGHT_GREEN, "%f\n", cost_min);

  // Table header
  Console::printf(Console::Color::BRIGHT_WHITE, "Level \u2502 ");
  // Assuming prune_keys are fixed, you might need to adjust this based on actual enum values
  Console::printf(Console::Color::BRIGHT_WHITE, "actuations \u2502 cost \u2502 pressures \u2502 levels \u2502 stability \u2502 Feasible\n",
                  Console::Color::BRIGHT_WHITE);

  // Separator line
  Console::printf(Console::Color::WHITE, "------+---------+------+-----------+--------+----------\n");

  // Data rows
  for (size_t h = 0; h < prunings.size(); h++)
  {
    // Level number
    Console::printf(Console::Color::YELLOW, "%d     | ", h);

    // Pruning counts for each reason
    std::string actuations = std::to_string(prunings[h].at(PruneReason::ACTUATIONS));
    std::string cost = std::to_string(prunings[h].at(PruneReason::COST));
    std::string pressures = std::to_string(prunings[h].at(PruneReason::PRESSURES));
    std::string levels = std::to_string(prunings[h].at(PruneReason::LEVELS));
    std::string stability = std::to_string(prunings[h].at(PruneReason::STABILITY));

    Console::printf(Console::Color::CYAN, "%s       | %s    | %s       | %s      | %s      | ", actuations.c_str(), cost.c_str(), pressures.c_str(),
                    levels.c_str(), stability.c_str());

    // Feasible solutions
    Console::printf(Console::Color::GREEN, "%d\n", feasible_counter[h]);
  }
}
