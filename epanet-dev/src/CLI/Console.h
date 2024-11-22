// src/CLI/ColorStream.h
#pragma once

#include <chrono>
#include <cstdarg> // For variadic functions
#include <ctime>
#include <fstream>
#include <iostream>
#include <string>

class Console
{
public:
  /**
   * @brief Enumeration for different color codes.
   */
  enum class Color
  {
    RESET = 0,
    RED = 31,
    GREEN = 32,
    YELLOW = 33,
    BLUE = 34,
    MAGENTA = 35,
    CYAN = 36,
    WHITE = 37,
    ORANGE = 38,
    BRIGHT_RED = 91,
    BRIGHT_GREEN = 92,
    BRIGHT_YELLOW = 93,
    BRIGHT_BLUE = 94,
    BRIGHT_MAGENTA = 95,
    BRIGHT_CYAN = 96,
    BRIGHT_WHITE = 97
  };

  static bool use_logger;
  static std::string file_name;
  static std::ofstream logger_file;

  static void open(int rank, bool use_file = true, bool verbose = false);

  static void close()
  {
    logger_file.close();
    use_logger = false;
  }

  /**
   * @brief Prints formatted text with the specified color.
   *
   * @param color Color to apply.
   * @param format Format string similar to printf.
   * @param ... Variable arguments corresponding to the format specifiers.
   */
  static void printf(Color color, const char *format, ...);

  static void hline(Color color, size_t length = 10);

private:
};
