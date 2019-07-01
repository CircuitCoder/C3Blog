#include <crow.h>
#include <curl/curl.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <thread>

#include "handlers/account.h"
#include "auth.h"
#include "util.h"
#include "config.h"
#include "middleware.h"
#include "storage.h"

namespace rj = rapidjson;

namespace C3 {

  // 0 for none
  // 1 for http
  // 2 for socks4
  // 3 for socks5
  uint8_t proxy_type = 0;
  std::string proxy;

  void setup_account_handler(const Config &c) {
    curl_global_init(CURL_GLOBAL_ALL);

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

    if(proxy_type != 0) std::cout << "Using proxy: " << proxy <<std::endl;
  }

  const std::string baseurl = "https://www.googleapis.com/oauth2/v3/tokeninfo?id_token=";

  size_t curl_handler(void *content, size_t size, size_t nmemb, void *userp) {
    ((std::string*) userp) -> append((char*) content, size*nmemb);
    return size*nmemb;
  }
  
  void handle_account_login(const crow::request &req, crow::response &res) {
    rj::Document body;
    if(body.Parse(req.body).HasParseError()
        || !body.IsObject()
        || !body.HasMember("token")
        || !body["token"].IsString()
        || !body.HasMember("sub")
        || !body["sub"].IsString()) {
      res.code = 400;
      res.end("400 Bad Request");
      return;
    }

    Middleware::context &cookieCtx = _app->template get_context<Middleware>(req);

    std::string token = body["token"].GetString();
    std::string sub = body["sub"].GetString();

    if(cookieCtx.session.signedIn && cookieCtx.session.uident == std::string("google,") + sub) {
      res.code = 200;

      rj::StringBuffer result;
      rj::Writer<rj::StringBuffer> writer(result);

      writer.StartObject();
      writer.Key("valid");
      writer.Bool(true);
      writer.Key("isAuthor");
      writer.Bool(cookieCtx.session.isAuthor);
      writer.EndObject();

      res.end(result.GetString());
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

        rj::Document jres;
        if(jres.Parse(buf).HasParseError() || !jres.IsObject()) {
          res.code = 500;
          res.end("500 Internal Error");
          return;
        }

        if(!jres.HasMember("email") || !jres["email"].IsString()) {
          if(jres.HasMember("error_description")) {
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

        if(!jres.HasMember("sub") || !jres["sub"].IsString() || jres["sub"].GetString() != sub) {
          res.code = 403;
          res.set_header("Content-Type", "application/json; charset=utf-8");
          res.end("{\"error_description\":\"Sub mismatch\"}");
          return;
        }

        std::string email = jres["email"].GetString();

        User u(User::UserType::uGoogle,
            sub,
            jres.HasMember("name")
              && jres["name"].IsString()
              ? jres["name"].GetString() : "",
            email,
            jres.HasMember("picture")
              && jres["picture"].IsString()
              ? jres["picture"].GetString() : "");

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

        rj::StringBuffer result;
        rj::Writer<rj::StringBuffer> writer(result);

        writer.StartObject();
        writer.Key("valid");
        writer.Bool(true);
        writer.Key("isAuthor");
        writer.Bool(cookieCtx.session.isAuthor);
        writer.EndObject();

        res.end(result.GetString());
        return;
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
    Middleware::context &cookieCtx = _app->template get_context<Middleware>(req);

    cookieCtx.session.isAuthor = false;
    cookieCtx.session.signedIn = false;
    cookieCtx.session.uident = "";
    cookieCtx.saveSession = true;

    res.end("{\"ok\": 0}");
  }

  void handle_account_query(const crow::request &req, crow::response &res, const std::string &uident) {
    res.end(get_user_str(uident));
  }
}
