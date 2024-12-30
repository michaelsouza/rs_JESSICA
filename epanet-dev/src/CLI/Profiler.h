// src/CLI/Profiler.h
#pragma once

#include "Console.h"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <mpi.h>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

class Profiler
{
public:
  static void push(const std::string &name)
  {
    // Create a new stack frame with current time
    callStack.push({name, std::chrono::high_resolution_clock::now()});
  }

  static void pop()
  {
    auto [name, start_time] = callStack.top();
    auto end_time = std::chrono::high_resolution_clock::now();

    // Add the duration to the profile
    profile[name] += std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

    callStack.pop();
  }

  static const std::unordered_map<std::string, std::chrono::microseconds> &getProfile()
  {
    return profile;
  }

  static void save(const std::string &fn)
  {
    // Get MPI rank
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Only rank 0 prints the summary message
    if (rank == 0)
    {
      Console::printf(Console::Color::BRIGHT_GREEN, "💾 Writing profile to file: %s\n", fn.c_str());
    }

    std::ofstream outfile(fn);

    outfile << "=== Profiling Results (Rank " << rank << ") ===\n";

    // Create vector of pairs to sort
    std::vector<std::pair<std::string, std::chrono::microseconds>> sorted_profile(profile.begin(), profile.end());

    // Sort by duration in descending order
    std::sort(sorted_profile.begin(), sorted_profile.end(), [](const auto &a, const auto &b) { return a.second > b.second; });

    // Get the maximum duration for calculating percentages
    auto max_duration = sorted_profile.front().second.count();

    // Set output format
    outfile << std::fixed << std::setprecision(2);

    for (const auto &[name, duration] : sorted_profile)
    {
      double ms = duration.count() / 1000.0;
      double percentage = (duration.count() * 100.0) / max_duration;

      outfile << std::left << std::setw(30) << name << ": " << std::right << std::setw(8) << ms << " ms" << " (" << std::setw(5) << percentage
              << "%)\n";
    }
    outfile << "=====================\n";
    outfile.close();
  }

private:
  struct StackFrame
  {
    std::string name;
    std::chrono::high_resolution_clock::time_point start_time;
  };

  static inline std::stack<StackFrame> callStack;
  static inline std::unordered_map<std::string, std::chrono::microseconds> profile;

  // Prevent instantiation
  Profiler() = delete;

  friend class ProfileScope;
};

class ProfileScope
{
public:
  explicit ProfileScope(const std::string &name) : name_(name)
  {
    Profiler::push(name_);
  }

  ~ProfileScope()
  {
    Profiler::pop();
  }

private:
  std::string name_;

  // Prevent copying and assignment
  ProfileScope(const ProfileScope &) = delete;
  ProfileScope &operator=(const ProfileScope &) = delete;
};
