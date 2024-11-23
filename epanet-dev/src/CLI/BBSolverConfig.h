// src/CLI/BBSolverConfig.h
#pragma once

#include <string>

class BBSolverConfig
{
public:
  BBSolverConfig(int argc, char *argv[]);

  std::string inpFile;
  int h_max = 24;
  int max_actuations = 3;
  bool verbose = false;
  bool save_project = false;
  bool use_logger = false;
  int h_threshold = 18;

private:
  void parse_args(int argc, char *argv[]);
};
