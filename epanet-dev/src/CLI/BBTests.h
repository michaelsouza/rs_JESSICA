// src/BBTests.h

#ifndef BBTESTS_H
#define BBTESTS_H

#include "Core/network.h"
#include "Core/project.h"
#include "Elements/pump.h"
#include "epanet3.h"

#include "BBCounter.h"
#include "BBSolver.h"
#include "BBStats.h"
#include "ColorStream.h" // Include ColorStream header
#include "Helper.h"
#include "Utils.h"

#include <algorithm>
#include <chrono>
#include <cstdlib> // For std::atoi
#include <cstring>
#include <fstream> // Required for checking file existence
#include <iostream>
#include <limits>
#include <map>
#include <numeric>
#include <string>
#include <vector>
#include <cmath>    // For std::abs
#include <cassert>  // For assert

bool test_cost() {
    // Default values
    const char *inpFile =
        "/home/michael/gitrepos/rs_JESSICA/networks/any-town.inp";
    int h_max = 24;
    int max_actuations = 3;
    bool verbose = true;

    // Check if the inpFile exists
    std::ifstream fileCheck(inpFile);
    if (!fileCheck) {
        ColorStream::println("Error: Input file " + std::string(inpFile) + " does not exist.", ColorStream::Color::RED);
        return false;
    }

    // Initialize nodes and tanks with placeholder IDs
    std::map<std::string, int> nodes = {{"55", 0}, {"90", 0}, {"170", 0}};
    std::map<std::string, int> tanks = {{"65", 0}, {"165", 0}, {"265", 0}};
    std::vector<std::string> pump_names = {"111", "222", "333"};

    // Retrieve node and tank IDs from the input file
    get_nodes_and_tanks_ids(inpFile, nodes, tanks, verbose);

    // Display nodes, pumps, and tanks information
    show_nodes_pumps_tanks(nodes, pump_names, tanks, verbose);

    // Initialize branch-and-bound counter and statistics
    BBCounter counter(nodes.size(), h_max, max_actuations, pump_names.size());
    BBStats stats(h_max, max_actuations);

    // Global optimal solution from Costa2015
    std::vector<int> y = {1, 2, 1, 2, 1, 1, 1, 1, 0, 0, 2, 2,
                          2, 2, 2, 1, 2, 1, 0, 0, 0, 2, 1, 0};
    for (int h = 1; h <= h_max; ++h) {
        counter.y[h] = y[h - 1];
        counter.h = h;
        bool updated = counter.update_x(verbose);
        if (!updated) {
            ColorStream::println("Error: Failed to update x for hour " + std::to_string(h) + ".", ColorStream::Color::RED);
            return false;
        }
        counter.show_xy(verbose);
    }

    bool is_feasible = process_node(inpFile, counter, stats, nodes, tanks, pump_names, verbose);
    if (!is_feasible) {
        ColorStream::println("Error: Process node returned infeasible solution.", ColorStream::Color::RED);
        return false;
    }

    // Output the computed cost_min
    printf("stats.cost_min: %.2f\n", stats.cost_min);

    // Define expected cost and tolerance
    const double expected_cost_min = 3578.66;
    const double tolerance = 0.01;

    // Check if stats.cost_min is within the expected range
    if (std::abs(stats.cost_min - expected_cost_min) > tolerance) {
        ColorStream::println("Test Failed: stats.cost_min (" + std::to_string(stats.cost_min) +
                             ") is not within " + std::to_string(tolerance) +
                             " of expected " + std::to_string(expected_cost_min) + ".", ColorStream::Color::RED);
        return false;
    }

    // If the check passes
    ColorStream::println("Test Passed: stats.cost_min (" + std::to_string(stats.cost_min) +
                         ") is within " + std::to_string(tolerance) +
                         " of expected " + std::to_string(expected_cost_min) + ".", ColorStream::Color::GREEN);

    return true;
}

void test_all() {
    if (!test_cost()) {
        ColorStream::println("One or more tests failed.", ColorStream::Color::RED);
        exit(EXIT_FAILURE);
    } else {
        ColorStream::println("All tests passed successfully!", ColorStream::Color::GREEN);
    }
}

#endif // BBTESTS_H
