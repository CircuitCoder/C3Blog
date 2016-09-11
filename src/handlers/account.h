#pragma once

#include <crow.h>
#include <json/json.h>
#include <curl/curl.h>
#include <thread>

#include "auth.h"
#include "util.h"
#include "config.h"

namespace C3 {

  extern Json::StreamWriterBuilder wbuilder;
  extern Json::CharReaderBuilder rbuilder;

  // 0 for none
  // 1 for http
  // 2 for socks4
  // 3 for socks5
  uint8_t proxy_type = 0;
  std::string proxy;

  void setup_account_handler(const Config &c) {
    if(c.proxy.compare(0, 9, "socks4://") == 0) {
      proxy = c.proxy.substr(9);
      proxy_type = 2;
    } else if(c.proxy.compare(0, 9, "socks5://") == 0) {
      proxy = c.proxy.substr(9);
      proxy_type = 3;
    } else if(c.proxy.compare(0, 7, "http://") == 0) {
      proxy = c.proxy.substr(7);
      proxy_type = 1;
    }

    std::cout << "Using proxy: " << proxy <<std::endl;
  }

  const std::string baseurl = "https://www.googleapis.com/oauth2/v3/tokeninfo?id_token=";

  size_t curl_handler(void *content, size_t size, size_t nmemb, void *userp) {
    ((std::string*) userp) -> append((char*) content, size*nmemb);
    return size*nmemb;
  }
  
  void handle_account_login(const crow::request &req, crow::response &res) {
    Json::Value body;
    if(!parseFromString(rbuilder, req.body, &body)
        || !body.isObject()
        || !body.isMember("token")
        || !body["token"].isString()
        || !body.isMember("sub")
        || !body["sub"].isString()) {
      res.code = 400;
      res.end("400 Bad Request");
      return;
    }

    context * ctx = (context *) req.middleware_context;
    Middleware::context &cookieCtx = ctx->get<Middleware>();

    std::string token = body["token"].asString();
    std::string sub = body["sub"].asString();

    if(cookieCtx.session.signedIn && cookieCtx.session.uident == std::string("google,") + sub) {
      res.code = 200;
      Json::Value v;

      v["valid"] = true;
      v["isAuthor"] = cookieCtx.session.isAuthor;
      res.end(Json::writeString(wbuilder, v));
      return;
    }

    auto verifier = [token, sub, &res, &cookieCtx]() -> void {
      CURL *curl = curl_easy_init();
      if(curl) {
        // Using google for right now
        std::string buf;

        curl_easy_setopt(curl, CURLOPT_URL, (baseurl + token).c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_handler);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);

        if(proxy_type != 0) {
          curl_easy_setopt(curl, CURLOPT_PROXY, proxy.c_str());
          if(proxy_type == 1) curl_easy_setopt(curl, CURLOPT_PROXYTYPE, CURLPROXY_HTTP);
          else if(proxy_type == 2) curl_easy_setopt(curl, CURLOPT_PROXYTYPE, CURLPROXY_SOCKS4);
          else if(proxy_type == 3) curl_easy_setopt(curl, CURLOPT_PROXYTYPE, CURLPROXY_SOCKS5);
        }

        CURLcode cres = curl_easy_perform(curl);

        if(cres != CURLE_OK) {
          res.code = 500;
          res.end("500 Internal Error");
          std::cerr<<curl_easy_strerror(cres)<<std::endl;
          return;
        }

        curl_easy_cleanup(curl);

        // No user database for right now

        Json::Value jres;
        if(!parseFromString(rbuilder, buf, &jres) || !jres.isObject()) {
          res.code = 500;
          res.end("500 Internal Error");
          return;
        }

        if(!jres.isMember("email") || !jres["email"].isString()) {
          if(jres.isMember("error_description")) {
            res.code = 403;
            res.set_header("Content-Type", "application/json; charset=utf-8");
            res.end(buf);
            return;
          } else {
            res.code = 500;
            res.end("500 Internal Error");
            return;
          }
        }

        if(!jres.isMember("sub") || !jres["sub"].isString() || jres["sub"].asString() != sub) {
          res.code = 403;
          res.set_header("Content-Type", "application/json; charset=utf-8");
          res.end("{\"error_description\":\"Sub mismatch\"}");
          return;
        }

        std::string email = jres["email"].asString();

        User u(User::UserType::uGoogle,
            sub,
            jres.isMember("name") && jres["name"].isString() ? jres["name"].asString() : "",
            email,
            jres.isMember("picture") && jres["picture"].isString() ? jres["picture"].asString() : "");

        if(!update_user(u)) {
          res.code = 500;
          res.set_header("Content-Type", "application/json; charset=utf-8");
          res.end("{\"error_description\":\"Unable to update your account\"}");
          return;
        }

        cookieCtx.session.isAuthor = Auth::isAuthor(email);
        cookieCtx.session.signedIn = true;
        cookieCtx.session.uident = std::string("google,") + sub;
        cookieCtx.saveSession = true;

        Json::Value v;

        v["valid"] = true;
        v["isAuthor"] = cookieCtx.session.isAuthor;

        res.end(Json::writeString(wbuilder, v));
      } else {
        res.code = 500;
        res.end("500 Internal Error");
        return;
      }
    };

    std::thread verifyThread(verifier);
    verifyThread.detach();
  }

  void handle_account_logout(const crow::request &req, crow::response &res) {
    context * ctx = (context *) req.middleware_context;
    Middleware::context &cookieCtx = ctx->get<Middleware>();

    cookieCtx.session.isAuthor = false;
    cookieCtx.session.signedIn = false;
    cookieCtx.session.uident = "";
    cookieCtx.saveSession = true;

    res.end("{\"ok\": 0}");
  }
}
