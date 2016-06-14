#pragma once

#include <crow.h>

#include "config.h"
#include "util.h"
#include "auth.h"

namespace C3 {
  std::string origins;

  struct Middleware {
    struct context {
      Auth::Session session;
      std::string sid;
      bool saveSession = false;
    };

    Middleware() { }

    template<typename AllContext>
    void before_handle(crow::request &req, crow::response &res, context& ctx, AllContext &ctxs) {
      res.set_header("Access-Control-Allow-Origin", origins);
      res.set_header("Access-Control-Allow-Credentials", "true");
      res.set_header("Access-Control-Allow-Methods", req.get_header_value("Access-Control-Request-Method"));
      res.set_header("Access-Control-Allow-Headers", req.get_header_value("Access-Control-Request-Headers"));

      // Initial session
      auto &pctx = ctxs.template get<crow::CookieParser>();
      std::string sid = pctx.get_cookie("c3_sid");
      if(sid == "") {
        sid = random_chars(64);
        pctx.set_cookie("c3_sid", sid);
      }

      ctx.sid = sid;
      ctx.session = Auth::getSession(sid);

      if(req.method == "OPTIONS"_method) {
        res.end();
        return;
      }
    }

    void after_handle(crow::request &req, crow::response &res, context& ctx) {
      if(ctx.saveSession) saveSession(ctx.sid, ctx.session);
    }
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
