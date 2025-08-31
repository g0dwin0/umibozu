#pragma once
#include <chrono>

#include "common.hpp"
struct Stopwatch {
  std::chrono::time_point<std::chrono::high_resolution_clock> start_time, end_time;
  std::chrono::duration<double, std::milli> duration;

  void start() { start_time = std::chrono::high_resolution_clock::now(); }
  void end() {
    end_time = std::chrono::high_resolution_clock::now();

    duration = end_time - start_time;
  }
};