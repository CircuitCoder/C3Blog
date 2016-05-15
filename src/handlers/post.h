#pragma once

#include <sstream>
#include <crow.h>
#include <json/json.h>

#include "storage.h"

Json::FastWriter writer;
Json::Reader reader;

namespace C3 {
  void handle_post_list(const crow::request &req, crow::response &res);
  void handle_post_list_page(const crow::request &req, crow::response &res, int page);
  void handle_post_read(const crow::request &req, crow::response &res, uint64_t id);
  void handle_post_create(const crow::request &req, crow::response &res);
  void handle_post_update(const crow::request &req, crow::response &res, uint64_t id);

  void handle_post_list(const crow::request &req, crow::response &res) {
    return handle_post_list_page(req, res, 1);
  }

  void handle_post_list_page(const crow::request &req, crow::response &res, int page) {
    Json::Value v;
    std::stringstream s;
    s<<"Hello, "<<page<<"!";
    v["msg"] = s.str();

    res.end(writer.write(v));
  }

  void handle_post_read(const crow::request &req, crow::response &res, uint64_t id) {
    try {
      std::string cont = get_post_str(id);
      res.end(cont);
      return;
    } catch(ReadExcept &e) {
      if(e == NotFound) {
        res.code = 404;
        res.end("404 Not Found");
      } else {
        res.code = 500;
        res.end("500 Internal Error");
      }
    } catch(...) {
      res.code = 500;
      res.end("500 Internal Error");
    }
  }

  void handle_post_create(const crow::request &req, crow::response &res) {
    try {
      std::cout<<req.body<<std::endl;
      uint64_t id = add_post(json_to_new_post(req.body));
      Json::Value v;
      v["id"] = id;
      res.end(writer.write(v));
    } catch(...) {
      res.code = 500;
      res.end("500 Internal Error");
    }
  }

  void handle_post_update(const crow::request &req, crow::response &res, uint64_t id) {
    try {
      std::cout<<req.body<<std::endl;
      update_post(id, json_to_post(req.body));
      res.end("{\"ok\":0}");
    } catch(...) {
      res.code = 500;
      res.end("500 Internal Error");
    }
  }
}
