#include <string>

#include <crow.h>

#include "../feed.h"

namespace C3 {
  void handle_feed(const crow::request &req, crow::response &res);

  void handle_feed(const crow::request &req, crow::response &res) {
    res.set_header("Content-Type", "application/atom+xml; charset=utf-8");
    res.end(Feed::fetch());
  }
}
