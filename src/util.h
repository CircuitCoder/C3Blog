#pragma once

#include <random>

namespace C3 {
  class Randomizer {
    private:
    std::random_device rd;
    std::mt19937_64 mt;
    std::uniform_int_distribution<uint64_t> dist;

    public:
    Randomizer(void);
    uint64_t next(void);
  };

  uint64_t current_time(void);
}
