#pragma once

#include <crow.h>
#include <string>

#include "config.h"

namespace rj = rapidjson;

namespace C3 {
  void setup_account_handler(const Config &c);
  void handle_account_login(const crow::request &req, crow::response &res);
  void handle_account_logout(const crow::request &req, crow::response &res);
  void handle_account_query(const crow::request &req, crow::response &res, const std::string &uident);
}
