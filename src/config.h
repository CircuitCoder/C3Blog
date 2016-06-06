#include <string>

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
  };

  Config read_config(const std::string &path);
};
