// src/CLI/Utils.h
#pragma once

#include <chrono>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

// Function to check error codes and exit if an error occurs
void CHK(int err, const std::string &message);

// Function to display a timer
void show_timer(int mpi_rank, unsigned int niter, int h, int done_loc, int done_all, double cost_best, std::vector<int> &y_best, int is_feasible,
                std::chrono::high_resolution_clock::time_point tic, int interval_niter, int interval_solver);


// Function to write a vector to a file
void write_vector(std::ofstream &ofs, const std::vector<int> &vec, const std::string vec_name);
