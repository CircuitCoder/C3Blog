#pragma once

#include <crow.h>

void handle_cors_preflight(const crow::request &req, crow::response &res) {
  res.set_header("Access-Control-Allow-Methods", req.get_header_value("Access-Control-Request-Methods"));
  res.set_header("Access-Control-Allow-Headers", req.get_header_value("Access-Control-Request-Headers"));
  res.end();
}

