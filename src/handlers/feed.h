#pragma once

#include <string>

#include <crow.h>

namespace C3 {
  void handle_feed(const crow::request &req, crow::response &res);
  void handle_sitemap(const crow::request &req, crow::response &res);
}
