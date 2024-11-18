// src/CLI/Helper.h
#pragma once

#include "BBCounter.h"
#include "BBStats.h"
#include "Core/network.h"
#include "Core/project.h"
#include "Elements/pattern.h"
#include "Elements/pump.h"
#include "Utils.h"
#include "epanet3.h"
#include <map>
#include <string>
#include <vector>

// Function declarations
void show_pattern(Pattern *p, const std::string &name);
void update_pumps_pattern_speed(std::vector<Pump *> pumps, BBCounter &counter,
                                bool verbose = false);
std::vector<Pump *> get_pumps(Network *nw,
                              const std::vector<std::string> &pump_names);
void show_pressures(bool is_feasible, const std::string &node_name,
                    double pressure, double threshold);
bool check_pressures(EN_Project p, std::map<std::string, int> &nodes,
                     bool verbose = false);
void show_levels(bool is_feasible, const std::string &tank_name, double level,
                 double level_min, double level_max);
bool check_levels(EN_Project p, std::map<std::string, int> &tanks,
                  bool verbose = false);
void show_stability(bool is_feasible, const std::string &tank_name,
                    double level, double initial_level);
bool check_stability(EN_Project p, std::map<std::string, int> &tanks,
                     bool verbose = false);
void get_nodes_and_tanks_ids(const char *inpFile,
                             std::map<std::string, int> &nodes,
                             std::map<std::string, int> &tanks,
                             bool verbose = false);
void show_nodes_pumps_tanks(const std::map<std::string, int> &nodes,
                            const std::vector<std::string> &pump_names,
                            const std::map<std::string, int> &tanks,
                            bool verbose = false);
double calc_cost(const std::vector<Pump *> &pumps);
bool process_node(const char *inpFile, BBCounter &counter, BBStats &stats,
                  const std::map<std::string, int> &nodes,
                  const std::map<std::string, int> &tanks,
                  const std::vector<std::string> &pump_names,
                  bool verbose = false);
