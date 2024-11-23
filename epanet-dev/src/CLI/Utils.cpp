// src/CLI/Utils.cpp
#include "Utils.h"
#include "Console.h"

#include <chrono>
#include <cstdlib>
#include <string>
#include <vector>

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
void show_timer(int mpi_rank, unsigned int niter, int h, int done_loc, int done_all, std::vector<int> &y, int is_feasible,
                std::chrono::high_resolution_clock::time_point tic, int interval_niter, int interval_solver)
{
  // Only show the timer every `interval` iterations
  if (mpi_rank == 0 && niter % interval_niter == 0)
  {
    auto toc = std::chrono::high_resolution_clock::now();
    double eta_secs = std::chrono::duration_cast<std::chrono::duration<double>>(toc - tic).count();
    double avg_time_per_iter_ms = eta_secs / niter * 1000;

    std::cout << "\r"; // Move to the beginning of the line
    Console::printf(Console::Color::BRIGHT_BLUE, "⏱  Iter: ");
    Console::printf(Console::Color::BRIGHT_YELLOW, "%d", niter);
    Console::printf(Console::Color::BRIGHT_BLUE, " | Time: ");
    Console::printf(Console::Color::BRIGHT_CYAN, "%.2f secs", eta_secs);
    Console::printf(Console::Color::BRIGHT_BLUE, " | Avg: ");
    Console::printf(Console::Color::BRIGHT_CYAN, "%.2f ms", avg_time_per_iter_ms);
  }

  if (niter % interval_solver == 0)
  {
    Console::printf(Console::Color::BRIGHT_BLUE, "\nRank[%d] h=%d, done_loc=%d, done_all=%d, is_feasible=%d\ny: [", mpi_rank, h, done_loc, done_all,
                    is_feasible);
    for (size_t i = 0; i < y.size(); ++i)
    {
      Console::printf(Console::Color::BRIGHT_CYAN, "%d", y[i]);
      if (i != y.size() - 1)
      {
        Console::printf(Console::Color::BRIGHT_BLUE, ", ");
      }
    }
    Console::printf(Console::Color::BRIGHT_BLUE, "]\n");
  }
}
