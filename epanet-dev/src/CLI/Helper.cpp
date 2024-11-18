// src/CLI/Helper.cpp
#include "Helper.h"
#include <algorithm>
#include <cstring>
#include <iostream>

using Epanet::Project;

// Function to display pattern information
void show_pattern(Pattern *p, const std::string &name) {
  std::string type_name;
  switch (p->type) {
  case Pattern::FIXED_PATTERN:
    type_name = "FIXED";
    break;
  case Pattern::VARIABLE_PATTERN:
    type_name = "VARIABLE";
    break;
  default:
    type_name = "UNKNOWN";
    break;
  }
  std::cout << name << "[" << type_name << ", " << p->size() << "]: [";
  for (int i = 0; i < p->size(); i++) {
    std::cout << p->factor(i) << " ";
  }
  std::cout << "]" << std::endl;
}

// Function to update pump speed patterns based on counter
void update_pumps_pattern_speed(std::vector<Pump *> pumps, BBCounter &counter,
                                bool verbose) {
  const std::vector<int> &x = counter.x;
  const int h = counter.h;

  size_t num_pumps = pumps.size();
  if (verbose) {
    printf("\nUpdating pumps speed: h=%d, num_pumps=%zu\n", h, num_pumps);
  }
  for (int i = 1; i <= h; i++) {
    for (size_t pump_id = 0; pump_id < num_pumps; pump_id++) {
      Pump *pump = pumps[pump_id];
      FixedPattern *pattern = dynamic_cast<FixedPattern *>(pump->speedPattern);
      if (!pattern) {
        std::cerr << "Error: Pump " << pump_id
                  << " does not have a FixedPattern speed pattern."
                  << std::endl;
        continue;
      }
      const int *xi = &x[counter.num_pumps * i];
      double factor_new = static_cast<double>(xi[pump_id]);
      const int factor_id = i - 1; // pattern index is 0-based
      double factor_old = pattern->factor(factor_id);
      pattern->setFactor(factor_id, factor_new);
      if (verbose) {
        std::cout << "   h[" << i << "]: pump[" << pump_id
                  << "]: " << factor_old << " -> " << factor_new << std::endl;
      }
    }
  }
}

// Function to retrieve pump objects from the network
std::vector<Pump *> get_pumps(Network *nw,
                              const std::vector<std::string> &pump_names) {
  std::vector<Pump *> pumps;
  for (const std::string &name : pump_names) {
    Pump *pump = dynamic_cast<Pump *>(nw->link(name));
    if (pump == nullptr)
      std::cout << "Pump " << name << " not found" << std::endl;
    else {
      pumps.push_back(pump);
    }
  }
  return pumps;
}

// Function to display pressure status
void show_pressures(bool is_feasible, const std::string &node_name,
                    double pressure, double threshold) {
  if (!is_feasible)
    printf("  ⚠️ node[%3s]: %.2f < %.2f\n", node_name.c_str(), pressure,
           threshold);
  else
    printf("  ✅ node[%3s]: %.2f >= %.2f\n", node_name.c_str(), pressure,
           threshold);
}

// Function to check node pressures
bool check_pressures(EN_Project p, std::map<std::string, int> &nodes,
                     bool verbose) {
  if (verbose) {
    std::cout << "\nChecking pressures: [";
    for (const auto &node : nodes)
      std::cout << node.first << " ";
    std::cout << "]" << std::endl;
  }
  std::map<std::string, double> thresholds = {
      {"55", 42}, {"90", 51}, {"170", 30}};
  bool all_ok = true;
  for (const auto &node : nodes) {
    const std::string &node_name = node.first;
    const int &node_id = node.second;
    double pressure;
    CHK(EN_getNodeValue(node_id, EN_PRESSURE, &pressure, p),
        "Get node pressure");
    bool is_feasible = pressure >= thresholds[node_name];
    if (!is_feasible) {
      all_ok = false;
    }
    if (verbose)
      show_pressures(is_feasible, node_name, pressure, thresholds[node_name]);
  }
  return all_ok;
}

// Function to display tank level status
void show_levels(bool is_feasible, const std::string &tank_name, double level,
                 double level_min, double level_max) {
  if (!is_feasible)
    printf("  ⚠️ tank[%3s]: %.2f not in [%.2f, %.2f]\n", tank_name.c_str(),
           level, level_min, level_max);
  else
    printf("  ✅ tank[%3s]: %.2f in [%.2f, %.2f]\n", tank_name.c_str(), level,
           level_min, level_max);
}

// Function to check tank levels
bool check_levels(EN_Project p, std::map<std::string, int> &tanks,
                  bool verbose) {
  if (verbose) {
    std::cout << "\nChecking levels: [";
    for (const auto &tank : tanks)
      std::cout << tank.first << " ";
    std::cout << "]" << std::endl;
  }
  const double level_min = 66.53;
  const double level_max = 71.53;
  bool all_ok = true;
  for (const auto &tank : tanks) {
    const int &tank_id = tank.second;
    double level;
    CHK(EN_getNodeValue(tank_id, EN_HEAD, &level, p), "Get tank level");
    if (level < level_min || level > level_max) {
      all_ok = false;
    }
    if (verbose)
      show_levels(level >= level_min && level <= level_max, tank.first, level,
                  level_min, level_max);
  }
  return all_ok;
}

// Function to display stability status
void show_stability(bool is_feasible, const std::string &tank_name,
                    double level, double initial_level) {
  if (!is_feasible)
    printf("  ⚠️ tank[%3s]: %.2f < %.2f\n", tank_name.c_str(), level,
           initial_level);
  else
    printf("  ✅ tank[%3s]: %.2f >= %.2f\n", tank_name.c_str(), level,
           initial_level);
}

// Function to check tank stability
bool check_stability(EN_Project p, std::map<std::string, int> &tanks,
                     bool verbose) {
  if (verbose) {
    std::cout << "\nChecking stability: [";
    for (const auto &tank : tanks)
      std::cout << tank.first << " ";
    std::cout << "]" << std::endl;
  }
  const double initial_level = 66.93;
  bool all_ok = true;
  for (const auto &tank : tanks) {
    const std::string &tank_name = tank.first;
    const int &tank_id = tank.second;
    double level;
    CHK(EN_getNodeValue(tank_id, EN_HEAD, &level, p), "Get tank level");
    if (level < initial_level) {
      all_ok = false;
    }
    if (verbose)
      show_stability(level >= initial_level, tank_name, level, initial_level);
  }
  return all_ok;
}

// Function to retrieve node and tank IDs from the input file
void get_nodes_and_tanks_ids(const char *inpFile,
                             std::map<std::string, int> &nodes,
                             std::map<std::string, int> &tanks, bool verbose) {
  EN_Project p = EN_createProject();
  CHK(EN_loadProject(inpFile, p), "Load project");

  // Find node IDs
  for (auto &node : nodes) {
    const std::string &node_name = node.first;
    int node_id;
    CHK(EN_getNodeIndex(const_cast<char *>(node_name.c_str()), &node_id, p),
        "Get node index");
    node.second = node_id;
  }

  // Find tank IDs
  for (auto &tank : tanks) {
    const std::string &tank_name = tank.first;
    int tank_id;
    CHK(EN_getNodeIndex(const_cast<char *>(tank_name.c_str()), &tank_id, p),
        "Get tank index");
    tank.second = tank_id;
  }

  EN_deleteProject(p);
}

// Function to display nodes, pumps, and tanks
void show_nodes_pumps_tanks(const std::map<std::string, int> &nodes,
                            const std::vector<std::string> &pump_names,
                            const std::map<std::string, int> &tanks,
                            bool verbose) {
  if (verbose) {
    std::cout << "\nNodes: [ ";
    for (const auto &node : nodes)
      std::cout << node.first << " ";
    std::cout << "]" << std::endl;

    std::cout << "Pumps: [ ";
    for (const std::string &pump_name : pump_names)
      std::cout << pump_name << " ";
    std::cout << "]" << std::endl;

    std::cout << "Tanks: [ ";
    for (const auto &tank : tanks)
      std::cout << tank.first << " ";
    std::cout << "]" << std::endl;
  }
}

// Function to calculate the total cost of pump operations
double calc_cost(const std::vector<Pump *> &pumps) {
  double cost = 0.0;
  for (Pump *pump : pumps) {
    cost += pump->pumpEnergy.getCost();
  }
  return cost;
}

// Function to process a node in the branch-and-bound tree
bool process_node(const char *inpFile, BBCounter &counter, BBStats &stats,
                  const std::map<std::string, int> &nodes,
                  const std::map<std::string, int> &tanks,
                  const std::vector<std::string> &pump_names, bool verbose) {
  bool is_feasible = true;
  int t = 0, dt = 0, t_max = 3600 * counter.h;
  double cost = 0.0;

  EN_Project p = EN_createProject();
  Project *prj = static_cast<Project *>(p);
  Network *nw = prj->getNetwork();

  CHK(EN_loadProject(inpFile, p), "Load project");
  CHK(EN_initSolver(EN_INITFLOW, p), "Initialize solver");

  std::vector<Pump *> pumps = get_pumps(nw, pump_names);

  update_pumps_pattern_speed(pumps, counter, verbose);

  do {
    // Run the solver
    CHK(EN_runSolver(&t, p), "Run solver");

    if (verbose) {
      printf("\nSimulation: t_max=%d, t: %d, dt: %d\n", t_max, t, dt);
    }

    // Check node pressures
    is_feasible = check_pressures(
        p, const_cast<std::map<std::string, int> &>(nodes), verbose);
    if (!is_feasible) {
      stats.record_pruning("pressures", counter.h);
      break;
    }

    // Check tank levels
    is_feasible = check_levels(
        p, const_cast<std::map<std::string, int> &>(tanks), verbose);
    if (!is_feasible) {
      stats.record_pruning("levels", counter.h);
      break;
    }

    // Check cost
    cost = calc_cost(pumps);
    is_feasible = cost < stats.cost_min;
    if (!is_feasible) {
      if (verbose) {
        printf("  ⚠️ cost: %.2f >= %.2f\n", cost, stats.cost_min);
      }
      stats.record_pruning("cost", counter.h);
      counter.jump_to_end();
      break;
    }

    // Advance the solver
    CHK(EN_advanceSolver(&dt, p), "Advance solver");

    // Check if we have reached the maximum simulation time
    if (t + dt > t_max) {
      break;
    }
  } while (dt > 0);

  // Check stability for the last hour
  if (is_feasible && counter.h == counter.h_max) {
    is_feasible = check_stability(
        p, const_cast<std::map<std::string, int> &>(tanks), verbose);
    if (!is_feasible) {
      stats.record_pruning("stability", counter.h);
    }
    stats.record_solution(cost, counter.y);
  }

  // Delete the project
  EN_deleteProject(p);

  return is_feasible;
}
