// src/CLI/ColorStream.h

#ifndef COLORSTREAM_H
#define COLORSTREAM_H

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
    BRIGHT_RED = 91,
    BRIGHT_GREEN = 92,
    BRIGHT_YELLOW = 93,
    BRIGHT_BLUE = 94,
    BRIGHT_MAGENTA = 95,
    BRIGHT_CYAN = 96,
    BRIGHT_WHITE = 97
  };

  /**
   * @brief Prints text with the specified color without a newline.
   *
   * @param text Text to print.
   * @param color Color to apply.
   */
  static void print(const std::string &text, Color color = Color::RESET);

  /**
   * @brief Prints text with the specified color followed by a newline.
   *
   * @param text Text to print.
   * @param color Color to apply.
   */
  static void println(const std::string &text, Color color = Color::RESET);
};

#endif // COLORSTREAM_H
