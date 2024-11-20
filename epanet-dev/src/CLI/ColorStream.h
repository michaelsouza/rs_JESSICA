// src/CLI/ColorStream.h
#pragma once

#include <cstdarg> // For variadic functions
#include <iostream>
#include <string>

class ColorStream
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

  /**
   * @brief Prints formatted text with the specified color.
   *
   * @param color Color to apply.
   * @param format Format string similar to printf.
   * @param ... Variable arguments corresponding to the format specifiers.
   */
  static void printf(Color color, const char *format, ...);
};
