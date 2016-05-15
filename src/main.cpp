#include <iostream>
#include <crow.h>

#include "storage.h"
#include "handler.h"
#include "middleware.h"
#include "util.h"

using namespace C3;

const int PORT = 3018;

int main() {

  /* Setup */
  setup_storage("./db");

  crow::App<Middleware> app;

  auto posts_dispatcher = [](const crow::request &req, crow::response &res) -> void {

    /* List all */
    if(req.method == "GET"_method) return handle_post_list(req, res);

    /* New post */
    else return handle_post_create(req, res);
  };

  auto post_dispatcher = [](const crow::request &req, crow::response &res, const uint64_t &id) -> void {
    /* List all */
    if(req.method == "GET"_method) return handle_post_read(req, res, id);

    /* Update post */
    else if(req.method == "POST"_method) return handle_post_update(req, res, id);

    /* Delete post */
    else return handle_post_delete(req, res, id);
  };

  CROW_ROUTE(app, "/posts").methods("GET"_method, "POST"_method)(posts_dispatcher);
  
  /* List page */
  CROW_ROUTE(app, "/posts/<int>").methods("GET"_method)(handle_post_list_page);

  CROW_ROUTE(app, "/post/<uint>").methods("GET"_method ,"POST"_method, "DELETE"_method)(post_dispatcher);

  app.port(PORT).multithreaded().run();
}
