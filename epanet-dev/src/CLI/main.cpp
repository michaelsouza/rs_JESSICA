/* EPANET 3
 *
 * Copyright (c) 2016 Open Water Analytics
 * Distributed under the MIT License (see the LICENSE file for details).
 *
 */

//! \file main.cpp
//! \brief The main function used to run EPANET from the command line.

#include "Core/network.h"
#include "Core/project.h"
#include "Elements/pattern.h"
#include "Elements/pump.h"
#include "epanet3.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <limits>
#include <map>
#include <numeric>
#include <vector>
using namespace std;
using namespace Epanet;
using Elements = std::map<std::string, int>;

void CHK(int err, const std::string &message) {
  if (err != 0) {
    std::cerr << "ERR: " << message << err << std::endl;
    exit(1);
  }
}

#include <iostream>
#include <string>

#include <iostream>
#include <string>

class ColorStream {
public:
  enum class Color {
    RESET = 0,
    RED = 31,
    GREEN = 32,
    YELLOW = 33,
    BLUE = 34,
    MAGENTA = 35,
    CYAN = 36,
    WHITE = 37,
    BRIGHT_RED = 91,
    BRIGHT_GREEN = 92,
    BRIGHT_YELLOW = 93,
    BRIGHT_BLUE = 94,
    BRIGHT_MAGENTA = 95,
    BRIGHT_CYAN = 96,
    BRIGHT_WHITE = 97
  };

  static void print(const std::string &text, Color color = Color::RESET) {
    if (color != Color::RESET) {
      std::cout << "\033[" << static_cast<int>(color) << "m" << text
                << "\033[0m";
    } else {
      std::cout << text;
    }
  }

  static void println(const std::string &text, Color color = Color::RESET) {
    print(text, color);
    std::cout << std::endl;
  }
};

void calc_actuations_csum(int *actuations_csum, const std::vector<int> &x,
                          int h) {
  // Set actuations_csum to 0
  std::fill(actuations_csum, actuations_csum + 3, 0);

  // Calculate the cumulative actuations up to hour h
  for (int i = 1; i < h; i++) {
    const int *x_old = &x[3 * (i - 1)];
    const int *x_new = &x[3 * i];
    for (int j = 0; j < 3; j++) {
      if (x_new[j] > x_old[j])
        ++actuations_csum[j];
    }
  }
}

class BBCounter {
public:
  BBCounter(int y_max, int h_max, int max_actuations, int num_pumps) {
    this->y_max = y_max;
    this->h_max = h_max;
    this->max_actuations = max_actuations;
    this->num_pumps = num_pumps;
    this->y = std::vector<int>(h_max + 1, 0);
    this->x = std::vector<int>(num_pumps * (h_max + 1), 0);
  }

  bool update_y(bool is_feasible) {
    if (this->h == 0 && !is_feasible)
      return false;

    if (is_feasible && this->h < this->h_max) {
      this->h++;
      this->y[this->h] = 0;
      return true;
    }

    if (this->y[this->h] == this->y_max) {
      this->y[this->h] = 0;
      this->h--;
      return this->update_y(false);
    }

    if (this->y[this->h] < this->y_max) {
      this->y[this->h]++;
      return true;
    }

    return false;
  }

  void jump_to_end() { this->y[this->h] = this->y_max; }

  bool update_x(bool verbose = false) {
    bool is_feasible = this->update_x_core(verbose);
    if (is_feasible) {
      // check consistency
      const int *x_new = &this->x[this->num_pumps * this->h];
      int sum_x = std::accumulate(x_new, x_new + this->num_pumps, 0);
      if (sum_x != this->y[this->h])
        throw std::runtime_error("sum(x)=" + std::to_string(sum_x) +
                                 " != y=" + std::to_string(this->y[this->h]));
    }
    return is_feasible;
  }

  void show_xy(bool verbose = false) {
    if (!verbose)
      return;
    std::cout << "\n";
    for (int i = 1; i <= this->h; i++) {
      printf("h[%2d]: y=%d, x=[", i, this->y[i]);
      const int *x_i = &this->x[this->num_pumps * i];
      for (int j = 0; j < this->num_pumps; j++) {
        printf("%d ", x_i[j]);
      }
      std::cout << "]" << std::endl;
    }
  }

  int h = 0;
  int y_max = 0;
  int h_max = 0;
  std::vector<int> y;
  std::vector<int> x;
  int max_actuations = 0;
  int num_pumps = 0;

private:
  bool update_x_core(bool verbose = false) {

    // Get handle variables
    const int h = this->h;
    const int max_actuations = this->max_actuations;
    const std::vector<int> &y = this->y;
    std::vector<int> &x = this->x;

    // Get the previous and new states
    const int &y_old = y[h - 1];
    const int &y_new = y[h];
    const int *x_old = &x[this->num_pumps * (h - 1)];
    int *x_new = &x[this->num_pumps * h];

    // Start by copying the previous state
    std::copy(x_old, x_old + this->num_pumps, x_new);

    if (y_new == y_old) {
      return true;
    }

    // Calculate the cumulative actuations up to hour h
    int actuations_csum[this->num_pumps];
    calc_actuations_csum(actuations_csum, x, h);

    // Get the sorted indices of the actuations_csum
    int pumps_sorted[this->num_pumps];
    std::iota(pumps_sorted, pumps_sorted + this->num_pumps, 0);
    std::sort(pumps_sorted, pumps_sorted + this->num_pumps, [&](int i, int j) {
      return actuations_csum[i] < actuations_csum[j];
    });

    if (y_new > y_old) {
      int num_actuations = y_new - y_old;
      // Identify pumps that are not currently actuating
      for (int i = 0; i < this->num_pumps && num_actuations > 0; i++) {
        const int pump = pumps_sorted[i];
        if (x_new[pump] == 0) {
          if (actuations_csum[pump] >= max_actuations)
            return false;
          x_new[pump] = 1;
          --num_actuations;
        }
      }
      return num_actuations == 0;
    }

    if (y_new < y_old) {
      int num_deactuations = y_old - y_new;
      for (int i = 0; i < this->num_pumps && num_deactuations > 0; i++) {
        const int pump = pumps_sorted[i];
        if (x_new[pump] == 1) {
          x_new[pump] = 0;
          --num_deactuations;
        }
      }
      return num_deactuations == 0;
    }

    return true;
  }
};

class BBStats {
public:
  BBStats(int h_max, int max_actuations) {
    this->cost_min = std::numeric_limits<double>::infinity();
    this->y_min = std::vector<int>(h_max + 1, 0);
    this->prunings = std::vector<std::map<std::string, int>>(h_max + 1);
    this->feasible_counter = std::vector<int>(h_max + 1, 0);

    // Prune Keys: actuations, cost, pressures, levels, stability
    this->prune_keys = {"actuations", "cost", "pressures", "levels",
                        "stability"};

    for (int h = 0; h <= h_max; h++) {
      this->feasible_counter[h] = 0;
      this->prunings[h] = std::map<std::string, int>();
      for (const std::string &key : this->prune_keys)
        this->prunings[h][key] = 0;
    }
  }
  void record_pruning(const std::string &reason, int h) {
    this->prunings[h][reason]++;
  }
  void record_feasible(int h) { this->feasible_counter[h]++; }
  void record_solution(double cost, const std::vector<int> &y) {
    if (cost < this->cost_min) {
      this->cost_min = cost;
      this->y_min = y;

      // Print cost in bright green
      ColorStream::println("\ncost_min: " + std::to_string(this->cost_min),
                           ColorStream::Color::BRIGHT_GREEN);

      // Print y_min in bright yellow
      ColorStream::print("y_min: [", ColorStream::Color::BRIGHT_YELLOW);
      for (size_t i = 1; i < this->y_min.size(); i++) {
        ColorStream::print(std::to_string(this->y_min[i]) + " ",
                           ColorStream::Color::BRIGHT_CYAN);
      }
      ColorStream::println("]", ColorStream::Color::BRIGHT_YELLOW);
    }
  }

  double cost_min;
  std::vector<int> y_min;
  std::vector<std::map<std::string, int>> prunings;
  std::vector<int> feasible_counter;
  std::vector<std::string> prune_keys;
};

void show_pattern(Pattern *p, std::string name) {
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

void update_pumps_pattern_speed(std::vector<Pump *> pumps, BBCounter &counter,
                                bool verbose = false) {
  const std::vector<int> &x = counter.x;
  const int h = counter.h;

  size_t num_pumps = pumps.size();
  if (verbose) {
    printf("\nUpdating pumps speed: h=%d, num_pumps=%zu\n", h, num_pumps);
  }
  for (int i = 1; i <= h; i++) {
    for (size_t pump_id = 0; pump_id < num_pumps; pump_id++) {
      Pump *pump = pumps[pump_id];
      FixedPattern *pattern = (FixedPattern *)pump->speedPattern;
      const int *xi = &x[3 * i];
      double factor_new = (double)xi[pump_id];
      const int factor_id = i - 1; // pattern index is 0-based
      const double factor_old = pattern->factor(factor_id);
      pattern->setFactor(factor_id, factor_new);
      if (verbose) {
        std::cout << "   h[" << i << "]: pump[" << pump_id
                  << "]: " << factor_old << " -> " << factor_new << std::endl;
      }
    }
  }
}

std::vector<Pump *> get_pumps(Network *nw,
                              std::vector<std::string> &pump_names) {
  std::vector<Pump *> pumps;
  for (const std::string &name : pump_names) {
    Pump *pump = (Pump *)nw->link(name);
    if (pump == nullptr)
      std::cout << "Pump " << name << " not found" << std::endl;
    else {
      pumps.push_back(pump);
    }
  }
  return pumps;
}

void show_pressures(bool is_feasible, std::string node_name, double pressure,
                    double threshold) {
  if (!is_feasible)
    printf("  ⚠️ node[%3s]: %.2f < %.2f\n", node_name.c_str(), pressure,
           threshold);
  else
    printf("  ✅ node[%3s]: %.2f >= %.2f\n", node_name.c_str(), pressure,
           threshold);
}

bool check_pressures(EN_Project p, Elements &nodes, bool verbose = false) {
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

void show_levels(bool is_feasible, std::string tank_name, double level,
                 double level_min, double level_max) {
  if (!is_feasible)
    printf("  ⚠️ tank[%3s]: %.2f not in [%.2f, %.2f]\n", tank_name.c_str(),
           level, level_min, level_max);
  else
    printf("  ✅ tank[%3s]: %.2f in [%.2f, %.2f]\n", tank_name.c_str(), level,
           level_min, level_max);
}

bool check_levels(EN_Project p, Elements &tanks, bool verbose = false) {
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

void show_stability(bool is_feasible, std::string tank_name, double level,
                    double initial_level) {
  if (!is_feasible)
    printf("  ⚠️ tank[%3s]: %.2f < %.2f\n", tank_name.c_str(), level,
           initial_level);
  else
    printf("  ✅ tank[%3s]: %.2f >= %.2f\n", tank_name.c_str(), level,
           initial_level);
}

bool check_stability(EN_Project p, Elements &tanks, bool verbose = false) {
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

void get_nodes_and_tanks_ids(const char *inpFile, Elements &nodes,
                             Elements &tanks, bool verbose = false) {
  EN_Project p = EN_createProject();
  CHK(EN_loadProject(inpFile, p), "Load project");

  // Find node ids
  for (auto &node : nodes) {
    const std::string &node_name = node.first;
    int node_id;
    CHK(EN_getNodeIndex(const_cast<char *>(node_name.c_str()), &node_id, p),
        "Get node index");
    node.second = node_id;
  }

  // Find tank ids
  for (auto &tank : tanks) {
    const std::string &tank_name = tank.first;
    int tank_id;
    CHK(EN_getNodeIndex(const_cast<char *>(tank_name.c_str()), &tank_id, p),
        "Get tank index");
    tank.second = tank_id;
  }

  EN_deleteProject(p);  
}

void show_timer(unsigned int niter,
                std::chrono::high_resolution_clock::time_point tic) {
  auto toc = std::chrono::high_resolution_clock::now();
  double elapsed_time =
      std::chrono::duration_cast<std::chrono::duration<double>>(toc - tic)
          .count();
  double avg_time_per_iter = elapsed_time / niter;

  std::cout << "\r"; // Move to the beginning of the line
  ColorStream::print("⏱  Iter: ", ColorStream::Color::BRIGHT_BLUE);
  ColorStream::print(std::to_string(niter), ColorStream::Color::BRIGHT_YELLOW);
  ColorStream::print(" | Time: ", ColorStream::Color::BRIGHT_BLUE);
  ColorStream::print(std::to_string(elapsed_time) + " s",
                     ColorStream::Color::BRIGHT_CYAN);
  ColorStream::print(" | Avg: ", ColorStream::Color::BRIGHT_BLUE);
  ColorStream::print(std::to_string(avg_time_per_iter) + " s",
                     ColorStream::Color::BRIGHT_CYAN);
  std::cout.flush();
}

double calc_cost(std::vector<Pump *> pumps) {
  double cost = 0.0;
  for (Pump *pump : pumps) {
    cost += pump->pumpEnergy.getCost();
  }
  return cost;
}

bool process_node(const char *inpFile, BBCounter &counter, BBStats &stats,
                  Elements &nodes, Elements &tanks,
                  std::vector<std::string> &pump_names, bool verbose = false) {
  bool is_feasible = true;
  int t = 0, dt = 0, t_max = 3600 * counter.h;
  double cost = 0.0;

  EN_Project p = EN_createProject();
  Project *prj = (Project *)p;
  Network *nw = prj->getNetwork();

  CHK(EN_loadProject(inpFile, p), "Load project");
  CHK(EN_initSolver(EN_INITFLOW, p), "Initialize solver");

  std::vector<Pump *> pumps = get_pumps(nw, pump_names);

  update_pumps_pattern_speed(pumps, counter, verbose);

  do {
    // run the solver
    CHK(EN_runSolver(&t, p), "Run solver");

    if (verbose) {
      printf("\nSimulation: t_max=%d, t: %d, dt: %d\n", t_max, t, dt);
    }

    // Check node pressures
    is_feasible = check_pressures(p, nodes, verbose);
    if (!is_feasible) {
      stats.record_pruning("pressures", counter.h);
      break;
    }

    // Check tank levels
    is_feasible = check_levels(p, tanks, verbose);
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
    is_feasible = check_stability(p, tanks, verbose);
    if (!is_feasible) {
      stats.record_pruning("stability", counter.h);
    }
    stats.record_solution(cost, counter.y);
  }

  // delete the project
  EN_deleteProject(p);

  return is_feasible;
}

void show_nodes_pumps_tanks(Elements &nodes,
                            std::vector<std::string> &pump_names,
                            Elements &tanks, bool verbose = false) {
  if (verbose) {
    std::cout << "\nNodes: [";
    for (const auto &node : nodes)
      std::cout << node.first << " ";
    std::cout << "]" << std::endl;

    std::cout << "Pumps: [";
    for (const std::string &pump_name : pump_names)
      std::cout << pump_name << " ";
    std::cout << "]" << std::endl;

    std::cout << "Tanks: [";
    for (const auto &tank : tanks)
      std::cout << tank.first << " ";
    std::cout << "]" << std::endl;
  }
}

int main() {
  const char *inpFile = "/home/michael/github/rs_JESSICA/networks/any-town.inp";
  const int h_max = 24;
  const bool verbose = false;
  const int max_actuations = 3;

  // Find node and tank ids
  Elements nodes = {{"55", 0}, {"90", 0}, {"170", 0}};
  Elements tanks = {{"65", 0}, {"165", 0}, {"265", 0}};
  std::vector<std::string> pump_names = {"111", "222", "333"};
  get_nodes_and_tanks_ids(inpFile, nodes, tanks, verbose);

  show_nodes_pumps_tanks(nodes, pump_names, tanks, verbose);

  BBCounter counter(nodes.size(), h_max, max_actuations, pump_names.size());
  BBStats stats(h_max, max_actuations);

  bool is_feasible = true;
  unsigned int niter = 0;

  auto tic = std::chrono::high_resolution_clock::now();

  while (counter.update_y(is_feasible)) {
    niter++;
    show_timer(niter, tic);

    is_feasible = counter.update_x(verbose);
    counter.show_xy(verbose);

    if (!is_feasible) {
      if (verbose) {
        printf("⚠️ actuations: %d >= %d\n", counter.y[counter.h],
               counter.max_actuations);
      }
      stats.record_pruning("actuations", counter.h);
      continue;
    }

    is_feasible = process_node(inpFile, counter, stats, nodes, tanks,
                               pump_names, verbose);
    if (verbose) {
      std::cout << "\nis_feasible: " << is_feasible << "\n" << std::endl;
    }
  }

  return EXIT_SUCCESS;
}
