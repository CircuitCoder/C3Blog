#pragma once

#include <random>
#include <string>
#include <vector>
#include <json/json.h>

namespace C3 {
  class Randomizer {
    private:
    std::random_device rd;
    std::mt19937_64 mt;
    std::uniform_int_distribution<uint64_t> dist;

    public:
    Randomizer(uint64_t a = 0, uint64_t b = std::numeric_limits<uint64_t>::max());
    uint64_t next(void);
  };

  uint64_t current_time(void);

  std::vector<std::string> split(const std::string &, char);

  std::string random_chars(int);

  std::string markdown(std::string);

  bool parseFromString(const Json::CharReaderBuilder &, const std::string &, Json::Value *v);

  namespace URLEncoding {
    std::string url_encode(std::string str);
    std::string url_decode(std::string str);
  }
}
