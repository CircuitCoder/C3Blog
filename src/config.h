#pragma once

#include <string>
#include <vector>

namespace C3 {
  class Config {
    public:
    Config();

    bool read(const std::string &path);

    // Server
    uint16_t server_port;
    bool server_multithreaded;

    // Database
    std::string db_path;

    // Security
    std::vector<std::string> security_origins;

    // User
    std::vector<std::string> user_authors;
  };

  Config read_config(const std::string &path);
};
