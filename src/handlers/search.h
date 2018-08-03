#pragma once

#include <string>
#include <crow.h>

#include "config.h"

namespace C3 {
  void setup_search_handler(const Config &c);

  void handle_search(const crow::request &req, crow::response &res, std::string str);
  void handle_search_page(const crow::request &req, crow::response &res, std::string str, uint64_t page);
}
