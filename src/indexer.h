#pragma once

#include <string>
#include <unordered_map>
#include <list>

#include "config.h"
#include "storage.h"

namespace C3 {
  namespace Index {
    void setup(const Config &c);

    void reindex(const Post& p);
    void reindex_all(void);
    void invalidate();
    std::unordered_map<std::string, std::list<std::pair<uint32_t, bool>>>
      generate(const std::string &title, const std::string &body);
    std::list<std::pair<uint64_t, std::list<std::tuple<uint32_t, uint32_t, bool>>>>
      search(std::string target);
  }
}
