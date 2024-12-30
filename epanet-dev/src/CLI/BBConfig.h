// src/CLI/BBConfig.h
#pragma once

#include <string>

class BBConfig
{
public:
  BBConfig(int argc, char *argv[]);

  void show() const;

  std::string inpFile;
  int h_max = 24;
  int max_actuations = 3;
  int level = 5;
  bool verbose = false;
  char fn_stats[256];
  char fn_best[256];
  char fn_profile[256];

private:
  void generateFilenames();
};
