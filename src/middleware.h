#pragma once

#include "crow.h"

namespace C3 {
  struct Middleware {
    struct context {};

    Middleware() { }

    void before_handle(crow::request &req, crow::response &res, context& ctx) { }
    void after_handle(crow::request &req, crow::response &res, context& ctx) { }
  };
}
