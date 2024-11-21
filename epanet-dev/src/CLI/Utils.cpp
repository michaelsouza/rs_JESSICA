// src/CLI/Utils.cpp
#include "Utils.h"
#include "Console.h"
#include <cstdlib>

// Implementation of CHK function
void CHK(int err, const std::string &message)
{
  if (err != 0)
  {
    std::cerr << "ERR: " << message << " " << err << std::endl;
    exit(1);
  }
}

// Implementation of show_timer
void show_timer(unsigned int niter, std::chrono::high_resolution_clock::time_point tic, int interval)
{
  // Only show the timer every `interval` iterations
  if (niter % interval != 0)
  {
    return;
  }
  auto toc = std::chrono::high_resolution_clock::now();
  double elapsed_time = std::chrono::duration_cast<std::chrono::duration<double>>(toc - tic).count();
  double avg_time_per_iter = elapsed_time / niter;

  std::cout << "\r"; // Move to the beginning of the line
  Console::printf(Console::Color::BRIGHT_BLUE, "⏱  Iter: ");
  Console::printf(Console::Color::BRIGHT_YELLOW, "%d", niter);
  Console::printf(Console::Color::BRIGHT_BLUE, " | Time: ");
  Console::printf(Console::Color::BRIGHT_CYAN, "%.2f s", elapsed_time);
  Console::printf(Console::Color::BRIGHT_BLUE, " | Avg: ");
  Console::printf(Console::Color::BRIGHT_CYAN, "%.2f s", avg_time_per_iter);
  std::cout.flush();
}
