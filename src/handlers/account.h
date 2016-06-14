#pragma once

#include <crow.h>
#include <json/json.h>
#include <curl/curl.h>
#include <thread>

namespace C3 {

  extern Json::FastWriter writer;
  extern Json::Reader reader;

  const std::string baseurl = "https://www.googleapis.com/oauth2/v3/tokeninfo?id_token=";

  size_t curl_handler(void *content, size_t size, size_t nmemb, void *userp) {
    ((std::string*) userp) -> append((char*) content, size*nmemb);
    return size*nmemb;
  }
  
  void handle_account_login(const crow::request &req, crow::response &res) {
    Json::Value body;
    if(!reader.parse(req.body, body) || !body.isObject() || !body.isMember("token") || !body["token"].isString()) {
      res.code = 400;
      res.end("400 Bad Request");
      return;
    }

    std::string token = body["token"].asString();

    auto verifier = [token, &res]() -> void {
      std::cout<<"VERIFIER"<<std::endl;
      CURL *curl = curl_easy_init();
      if(curl) {
        // Using google for right now
        std::string buf;

        curl_easy_setopt(curl, CURLOPT_URL, (baseurl + token).c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_handler);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);

        CURLcode cres = curl_easy_perform(curl);

        if(cres != CURLE_OK) {
          res.code = 500;
          res.end("500 Internal Error");
          std::cerr<<curl_easy_strerror(cres)<<std::endl;
          return;
        }

        std::cout<<buf<<std::endl;

        res.end("{}");

        curl_easy_cleanup(curl);
      } else {
        res.code = 500;
        res.end("500 Internal Error");
        return;
      }
    };

    std::thread verifyThread(verifier);
    verifyThread.detach();
  }

  void handle_account_logout(const crow::request &req, crow::response &res);
}
