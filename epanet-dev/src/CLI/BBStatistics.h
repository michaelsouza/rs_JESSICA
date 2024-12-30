// src/CLI/BBStatistics.h
#pragma once

#include "CLI/BBConfig.h"
#include "CLI/BBConstraints.h"

#include <fstream>
#include <nlohmann/json.hpp>
#include <omp.h>
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
    data[SNAPSHOTS] = std::vector<int>(config.h_max + 1, 0);
    data[ACTUATIONS] = std::vector<int>(config.h_max + 1, 0);

    labels[NONE] = "NONE";
    labels[PRESSURES] = "PRESSURES";
    labels[LEVELS] = "LEVELS";
    labels[STABILITY] = "STABILITY";
    labels[COST] = "COST";
    labels[SNAPSHOTS] = "SNAPSHOTS";
    labels[ACTUATIONS] = "ACTUATIONS";
  }
  ~BBStatistics()
  {
  }

  inline void add_stats(BBPruneReason reason, int h)
  {
    data[reason][h]++;
  }

  void to_json(const std::string &fn) const
  {
    Console::printf(Console::Color::BRIGHT_GREEN, "💾 Writing statistics to file: %s\n", fn.c_str());
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
    int tid = omp_get_thread_num();
    Console::hline(Console::Color::BRIGHT_YELLOW, 20);
    Console::printf(Console::Color::BRIGHT_YELLOW, "TID[%d]: Statistics\n", omp_get_thread_num());
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
