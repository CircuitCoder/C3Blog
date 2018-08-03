#include <crow.h>

#include <unordered_set>

#include "config.h"
#include "util.h"
#include "auth.h"
#include "middleware.h"

namespace C3 {
  std::unordered_set<std::string> middleware_origins;

  void Middleware::after_handle(crow::request &req, crow::response &res, Middleware::context& ctx) {
    if(ctx.saveSession) saveSession(ctx.sid, ctx.session);

    // Append Content-Type if not presents
    if(res.get_header_value("Content-Type") == "") {
      if(res.code == 200)
        res.set_header("Content-Type", "application/json; charset=utf-8");
      else
        res.set_header("Content-Type", "text/plain; charset=utf-8");
    }
  }

  void setup_middleware(const Config &c) {
    middleware_origins.clear();
    middleware_origins.insert(c.security_origins.begin(), c.security_origins.end());
  }
}
