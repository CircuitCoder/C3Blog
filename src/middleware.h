#pragma once

#include <crow.h>

#include <unordered_set>

#include "config.h"
#include "util.h"
#include "auth.h"

namespace C3 {
  extern std::unordered_set<std::string> middleware_origins;

  struct Middleware {
    struct context {
      Auth::Session session;
      std::string sid;
      bool saveSession = false;
    };

    Middleware() { }

    template<typename AllContext>
    void before_handle(crow::request &req, crow::response &res, context& ctx, AllContext &ctxs) {

      std::string origin = req.get_header_value("Origin");
      if(origin.length() > 0 && middleware_origins.count(origin) > 0)
        res.set_header("Access-Control-Allow-Origin", origin);

      res.set_header("Access-Control-Allow-Credentials", "true");
      res.set_header("Access-Control-Allow-Methods", req.get_header_value("Access-Control-Request-Method"));
      res.set_header("Access-Control-Allow-Headers", req.get_header_value("Access-Control-Request-Headers"));

      if(req.method == "OPTIONS"_method) {
        res.end();
        return;
      }

      // Initial session
      auto &pctx = ctxs.template get<crow::CookieParser>();
      std::string sid = pctx.get_cookie("c3_sid");

      bool newSession = sid == "";

      if(!newSession) {
        try {
          ctx.session = Auth::getSession(sid);
        } catch(Auth::AuthError e) {
          if(e == Auth::AuthError::NotSignedIn) newSession = true;
          else throw;
        }
      }

      if(newSession) {
        // Renew sid
        sid = random_chars(64);
        pctx.set_cookie("c3_sid", sid + "; Path=/");
        ctx.saveSession = true;
      }

      ctx.sid = sid;
    }

    void after_handle(crow::request &req, crow::response &res, context& ctx);
  };

  void setup_middleware(const Config &c);
}

typedef crow::App<crow::CookieParser, C3::Middleware> App;
extern std::unique_ptr<App> _app;
