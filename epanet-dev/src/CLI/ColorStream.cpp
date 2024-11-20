// src/CLI/ColorStream.cpp

#include "ColorStream.h"
#include <cstdio> // For vsnprintf
#include <memory> // For std::unique_ptr
#include <vector> // For dynamic buffer

void ColorStream::printf(Color color, const char *format, ...)
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
}
