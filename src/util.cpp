#include <cstdint>
#include <sstream>
#include <chrono>

#include "util.h"

namespace C3 {
  Randomizer::Randomizer(void) : mt(rd()) { }

  uint64_t Randomizer::next(void) {
    return dist(mt);
  }

  uint64_t current_time(void) {
    return std::chrono::duration_cast<std::chrono::duration<uint64_t, std::nano>>
      (std::chrono::system_clock::now().time_since_epoch()).count();
  }

  std::vector<std::string> split(const std::string &s, char delim) {
    //TODO: use list
    std::vector<std::string> elems;
    std::stringstream ss(s);
    std::string item;
    while(std::getline(ss, item, delim)) elems.push_back(item);
    return elems;
  }
}
