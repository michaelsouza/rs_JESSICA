// src/CLI/Utils.h
#pragma once

#include <chrono>
#include <iostream>
#include <string>

// Function to check error codes and exit if an error occurs
void CHK(int err, const std::string &message);

// Function to display a timer
void show_timer(unsigned int niter, std::chrono::high_resolution_clock::time_point tic, int interval = 100);
