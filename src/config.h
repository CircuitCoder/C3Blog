#pragma once

#include <string>
#include <vector>

namespace C3 {
  class Config {
    public:
    Config();

    bool read(const std::string &path);

    // App
    std::string app_title;
    std::string app_url;
    uint16_t app_feedLength;
    uint16_t app_pageLength;

    // Server
    uint16_t server_port;
    bool server_multithreaded;

    // Proxy
    std::string proxy;

    // Search
    std::string search_dict;
    uint16_t search_cache;

    // Database
    std::string db_path;
    uint64_t db_cache;

    // Security
    std::vector<std::string> security_origins;

    // User
    std::vector<std::string> user_authors;
  };

  Config read_config(const std::string &path);
};
