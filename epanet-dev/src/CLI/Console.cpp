// src/CLI/ColorStream.cpp

#include "Console.h"
#include <cstdio> // For vsnprintf
#include <memory> // For std::unique_ptr
#include <vector> // For dynamic buffer

bool Console::use_file = false;
std::string Console::file_name;
std::ofstream Console::output_file;

void Console::open(int rank, bool use_file, bool verbose)
{
  Console::use_file = use_file;
  if (!use_file) return;

  file_name = "logger_RANK_" + std::to_string(rank) + ".log";

  output_file.open(file_name);
  if (!output_file.is_open())
  {
    std::cerr << "Failed to open file: " << file_name << std::endl;
    exit(EXIT_FAILURE);
  }
  if (verbose) std::cout << "Logging to " << file_name << std::endl;
}

void Console::printf(Color color, const char *format, ...)
{
  // Initialize a variable argument list
  va_list args1;
  va_start(args1, format);

  // Copy args1 to args2 for measuring the required buffer size
  va_list args2;
  va_copy(args2, args1);

  // Determine the size needed for the formatted string
  int length = vsnprintf(nullptr, 0, format, args1);
  va_end(args1);

  if (length < 0)
  {
    // Handle encoding errors
    std::cerr << "Error formatting string." << std::endl;
    va_end(args2);
    return;
  }

  // Allocate buffer of the required size
  std::unique_ptr<char[]> buffer(new char[length + 1]);

  // Format the string
  vsnprintf(buffer.get(), length + 1, format, args2);
  va_end(args2);

  // Create the colored string
  if (color != Color::RESET)
  {
    std::cout << "\033[" << static_cast<int>(color) << "m" << buffer.get() << "\033[0m";
  }
  else
  {
    std::cout << buffer.get();
  }

  if (use_file)
  {
    output_file << buffer.get();
    output_file.flush();
  }
}

void Console::hline(Color color, size_t length)
{
  // Create buffer for the line
  std::string buffer;
  buffer.reserve(length + 1);
  for (size_t i = 0; i < length; i++)
  {
    buffer += "\u2550";
  }
  buffer += "\n";

  // Output to console with color
  if (color != Color::RESET)
  {
    std::cout << "\033[" << static_cast<int>(color) << "m" << buffer << "\033[0m";
  }
  else
  {
    std::cout << buffer;
  }

  // Write to file if enabled
  if (use_file)
  {
    output_file << buffer;
    output_file.flush();
  }
}
