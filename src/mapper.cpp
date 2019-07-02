#include "mapper.h"

#include <string>
#include <map>
#include <stdexcept>

namespace C3 {
  std::map<std::string, uint64_t> urlMap;

  bool has_url(const std::string &url) {
    return urlMap.count(url) > 0;
  }

  void add_url(const std::string &url, uint64_t id) {
    auto res = urlMap.insert(std::make_pair(url, id));
    if(!res.second) throw MapperError::DuplicatedUrl;
  }

  uint64_t query_url(const std::string &url) {
    try {
      return urlMap.at(url);
    } catch(std::out_of_range &e) {
      throw MapperError::UrlNotFound;
    }
  }

  void rename_url(const std::string &from, const std::string &to, uint64_t validator) {
    uint64_t original;
    try {
      original = urlMap.at(from);
    } catch(std::out_of_range &e) {
      throw MapperError::UrlNotFound;
    }

    if(validator != original) throw MapperError::ValidationFailed;

    auto res = urlMap.insert(std::make_pair(to, original));
    if(!res.second) throw MapperError::DuplicatedUrl;

    urlMap.erase(from);
  }

  void remove_url(const std::string &url) {
    if(urlMap.erase(url) == 0) throw MapperError::UrlNotFound;
  }
}
