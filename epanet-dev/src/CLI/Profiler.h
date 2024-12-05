// src/CLI/Profiler.h
#pragma once

#include "Console.h"
#include <chrono>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>

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

  static void print()
  {
    Console::printf(Console::Color::BRIGHT_BLUE, "\n=== Profiling Results ===\n");
    
    // Create vector of pairs to sort
    std::vector<std::pair<std::string, std::chrono::microseconds>> sorted_profile(profile.begin(), profile.end());
    
    // Sort by duration in descending order
    std::sort(sorted_profile.begin(), sorted_profile.end(),
              [](const auto& a, const auto& b) {
                return a.second > b.second;
              });
    
    for (const auto &[name, duration] : sorted_profile)
    {
      double ms = duration.count() / 1000.0;
      Console::printf(Console::Color::BRIGHT_CYAN, "%-30s", name.c_str());
      Console::printf(Console::Color::BRIGHT_WHITE, ": %8.2f ms\n", ms);
    }
    Console::printf(Console::Color::BRIGHT_BLUE, "=====================\n");
  }

private:
  struct StackFrame {
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
