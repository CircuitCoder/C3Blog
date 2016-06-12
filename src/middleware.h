#pragma once

#include <crow.h>

#include "config.h"

namespace C3 {
  std::string origins;

  struct Middleware {
    struct context {};

    Middleware() { }

    void before_handle(crow::request &req, crow::response &res, context& ctx) {
      res.set_header("Access-Control-Allow-Origin", origins);
      res.set_header("Access-Control-Allow-Credentials", "true");
    }
    void after_handle(crow::request &req, crow::response &res, context& ctx) { }
  };

  void setup_middleware(const Config &c) {
    std::stringstream originsStream;

    for(size_t i = 0; i < c.security_origins.size(); ++i) {
      originsStream<<c.security_origins[i];
      if(i != c.security_origins.size() -1) originsStream<<',';
    }

    origins = originsStream.str();
  }
}
