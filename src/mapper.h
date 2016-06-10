#pragma once

#include <string>

namespace C3 {
  enum class MapperError {
    UrlNotFound, DuplicatedUrl, ValidationFailed
  };

  bool has_url(const std::string &url);
  void add_url(const std::string &url, uint64_t id);
  uint64_t query_url(const std::string &url);
  void rename_url(const std::string &from, const std::string &to, uint64_t validator);
  void remove_url(const std::string &url);
}
