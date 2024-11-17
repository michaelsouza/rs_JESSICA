// src/CLI/Utils.cpp
#include "Utils.h"
#include "ColorStream.h"
#include <cstdlib>

// Implementation of CHK function
void CHK(int err, const std::string &message) {
    if (err != 0) {
        std::cerr << "ERR: " << message << " " << err << std::endl;
        exit(1);
    }
}

// Implementation of show_timer
void show_timer(unsigned int niter, std::chrono::high_resolution_clock::time_point tic) {
    auto toc = std::chrono::high_resolution_clock::now();
    double elapsed_time =
        std::chrono::duration_cast<std::chrono::duration<double>>(toc - tic)
            .count();
    double avg_time_per_iter = elapsed_time / niter;

    std::cout << "\r"; // Move to the beginning of the line
    ColorStream::print("⏱  Iter: ", ColorStream::Color::BRIGHT_BLUE);
    ColorStream::print(std::to_string(niter), ColorStream::Color::BRIGHT_YELLOW);
    ColorStream::print(" | Time: ", ColorStream::Color::BRIGHT_BLUE);
    ColorStream::print(std::to_string(elapsed_time) + " s",
                       ColorStream::Color::BRIGHT_CYAN);
    ColorStream::print(" | Avg: ", ColorStream::Color::BRIGHT_BLUE);
    ColorStream::print(std::to_string(avg_time_per_iter) + " s",
                       ColorStream::Color::BRIGHT_CYAN);
    std::cout.flush();
}
