// src/CLI/BBConfig.h
#pragma once

#include <string>

class BBConfig
{
public:
  BBConfig(int argc, char *argv[]);

  void show() const;

  std::string inpFile;
  int num_threads = 1;
  int h_max = 24;
  int h_min = 18;
  int max_actuations = 3;
  bool verbose = false;
};
