// src/CLI/BBConfig.h
#pragma once

#include <string>

class BBConfig
{
public:
  BBConfig(int argc, char *argv[]);

  std::string inpFile;
  int h_max = 24;
  // Maximum number of times a pump can change state from 0 to 1 (i.e. turn ons)
  // during the simulation period
  int max_actuations = 3;
  bool verbose = false;
  bool save_project = false;
  bool use_logger = false;
  int h_threshold = 18;

private:
  void parse_args(int argc, char *argv[]);
};
