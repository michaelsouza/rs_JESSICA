// src/CLI/BBConfig.h
#pragma once

#include <string>

class BBConfig
{
public:
  BBConfig(int argc, char *argv[]);

  void show() const;

  std::string inpFile;
  int num_threads = 2;
  int h_max = 24;
  int max_actuations = 3;
  int max_tasks = 256; // max number of tasks in queue
  bool verbose = false;
  std::string fn_stats;
  std::string fn_best;
};
