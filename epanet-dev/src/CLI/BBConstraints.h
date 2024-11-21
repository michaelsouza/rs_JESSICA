// src/CLI/BBConstraints.h
#pragma once

#include "Core/network.h"
#include "Core/project.h"
#include "Elements/pump.h"
#include "epanet3.h"
#include <map>
#include <string>

using Epanet::Project;

class BBConstraints
{
public:
  BBConstraints(std::string inpFile);
  ~BBConstraints();

  // Function to check node pressures
  bool check_pressures(bool verbose = false);

  // Function to check tank levels
  bool check_levels(bool verbose = false);

  // Function to check tank stability
  bool check_stability(bool verbose = false);

  // Function to check the cost
  bool check_cost(const double cost, bool verbose = false);

  void get_nodes_and_tanks_ids(std::string inpFile);

  // Function to calculate the total cost of pump operations
  double calc_cost() const;

  int get_num_nodes() const
  {
    return (int)nodes.size();
  }
  int get_num_tanks() const
  {
    return (int)tanks.size();
  }
  int get_num_pumps() const
  {
    return (int)pump_names.size();
  }

  void set_project(EN_Project p, const int h, const std::vector<int> &x, bool verbose)
  {
    this->p = p;
    Network *nw = static_cast<Project *>(p)->getNetwork();
    set_pumps(nw, h, x, verbose);
  }

  std::vector<std::string> pump_names;
  std::map<std::string, int> nodes;
  std::map<std::string, int> tanks;
  std::vector<Pump *> pumps;
  double cost_max;
  EN_Project p;
  std::string inpFile;

private:
  // Helper functions for displaying status
  void show_pressures(bool is_feasible, const std::string &node_name, double pressure, double threshold);
  void show_levels(bool is_feasible, const std::string &tank_name, double level, double level_min, double level_max);
  void show_stability(bool is_feasible, const std::string &tank_name, double level, double initial_level);
  void show_nodes_pumps_and_tanks(bool verbose);
  void set_pumps(Network *nw, const int h, const std::vector<int> &x, bool verbose = false);
  void update_pumps(const int h, const std::vector<int> &x, bool verbose);
};
