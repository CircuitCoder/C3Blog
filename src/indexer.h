#pragma once

#include <string>
#include <unordered_map>
#include <list>

#include "config.h"
#include "storage.h"

namespace C3 {
  void setup_indexer(const Config &c);

  void reindex(const Post& p);
  void reindex_all(void);
  void invalidate_index_cache(const uint64_t postid);
  std::unordered_map<std::string, std::list<std::pair<uint32_t, bool>>>
    generate_indexes(const std::string &title, const std::string &body);
  std::list<std::pair<uint64_t, std::list<std::tuple<uint32_t, uint32_t, bool>>>>
    search_index(std::string target);
}
