#pragma once

#include <crow.h>
#include <string>

#include "config.h"

namespace C3 {
  void setup_post_handler(const Config &c);
  void handle_post_list(const crow::request &req, crow::response &res);
  void handle_post_list_page(const crow::request &req, crow::response &res, int page);
  void handle_post_tag_list(const crow::request &req, crow::response &res, const std::string &tag);
  void handle_post_tag_list_page(const crow::request &req, crow::response &res, const std::string &tag, int page);
  void handle_post_read(const crow::request &req, crow::response &res, uint64_t id);
  void handle_post_read_url(const crow::request &req, crow::response &res, const std::string &url);
  void handle_post_create(const crow::request &req, crow::response &res);
  void handle_post_update(const crow::request &req, crow::response &res, uint64_t id);
  void handle_post_delete(const crow::request &req, crow::response &res, uint64_t id);
}
