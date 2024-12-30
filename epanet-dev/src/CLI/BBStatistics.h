// src/CLI/BBStatistics.h
#pragma once

#include "CLI/BBConfig.h"
#include "CLI/BBConstraints.h"

#include <fstream>
#include <mpi.h>
#include <nlohmann/json.hpp>
#include <string>
class BBStatistics
{
public:
  std::map<BBPruneReason, std::vector<int>> data;
  std::map<BBPruneReason, std::string> labels;
  double duration;

  BBStatistics(const BBConfig &config)
  {
    data[NONE] = std::vector<int>(config.h_max + 1, 0);
    data[PRESSURES] = std::vector<int>(config.h_max + 1, 0);
    data[LEVELS] = std::vector<int>(config.h_max + 1, 0);
    data[STABILITY] = std::vector<int>(config.h_max + 1, 0);
    data[COST] = std::vector<int>(config.h_max + 1, 0);
    data[ACTUATIONS] = std::vector<int>(config.h_max + 1, 0);

    labels[NONE] = "NONE";
    labels[PRESSURES] = "PRESSURES";
    labels[LEVELS] = "LEVELS";
    labels[STABILITY] = "STABILITY";
    labels[COST] = "COST";
    labels[ACTUATIONS] = "ACTUATIONS";
  }
  ~BBStatistics()
  {
  }

  inline void add_stats(BBPruneReason reason, int h)
  {
    data[reason][h]++;
  }

  void to_json(char *fn) const
  {
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0)
    {
      Console::printf(Console::Color::BRIGHT_GREEN, "💾 Writing statistics to file: %s\n", fn);
    }
    nlohmann::json j;
    for (const auto &[reason, counts] : data)
    {
      j[labels.at(reason)] = counts;
    }
    j["duration"] = duration;
    std::ofstream f(fn);
    f << j.dump(2);
  }

  void merge(const BBStatistics &other)
  {
    for (const auto &[reason, counts] : other.data)
    {
      // sum counts
      for (int h = 0; h < counts.size(); ++h)
      {
        data[reason][h] += counts[h];
      }
    }
  }

  void show() const
  {
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    Console::hline(Console::Color::BRIGHT_YELLOW, 20);
    Console::printf(Console::Color::BRIGHT_YELLOW, "TID[%d]: Statistics\n", rank);
    Console::printf(Console::Color::BRIGHT_YELLOW, "Duration: %.3f seconds\n", duration);
    for (const auto &[type, counts] : data)
    {
      Console::printf(Console::Color::CYAN, "%10s: [", labels.at(type).c_str());
      for (int i = 0; i < counts.size(); ++i)
      {
        Console::printf(Console::Color::CYAN, "%d, ", counts[i]);
      }
      Console::printf(Console::Color::CYAN, "]\n");
    }
  }
};
