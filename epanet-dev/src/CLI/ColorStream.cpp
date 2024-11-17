// src/CLI/ColorStream.cpp
#include "ColorStream.h"

void ColorStream::print(const std::string &text, Color color) {
    if (color != Color::RESET) {
        std::cout << "\033[" << static_cast<int>(color) << "m" << text << "\033[0m";
    } else {
        std::cout << text;
    }
}

void ColorStream::println(const std::string &text, Color color) {
    print(text, color);
    std::cout << std::endl;
}
