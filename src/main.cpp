#include <iostream>
#include <crow.h>
#include <signal.h>

#include "storage.h"
#include "handler.h"
#include "middleware.h"
#include "util.h"
#include "config.h"

using namespace C3;

const int PORT = 3018;

void stop_handler(int s) {
  std::cout<<"Shutdown down..."<<std::endl;
  stop_storage();
}


int main() {

  struct sigaction sigIntHandler;

  sigIntHandler.sa_handler = stop_handler;
  sigemptyset(&sigIntHandler.sa_mask);
  sigIntHandler.sa_flags = 0;

  sigaction(SIGINT, &sigIntHandler, NULL);

  /* Setup */
  Config c;
  if(!c.read("config.yml")) {
    std::cout<<"Failed to parse config file. Aborting."<<std::endl;
    return -1;
  }

  if(!setup_storage(c.db_path)) {
    std::cout<<"Failed to initialize storage. Aborting."<<std::endl;
    return -1;
  }

  setup_middleware(c);
  setup_url_map();

  crow::App<Middleware> app;

  auto posts_dispatcher = [](const crow::request &req, crow::response &res) -> void {

    /* List all */
    if(req.method == "GET"_method) return handle_post_list(req, res);

    /* New post */
    else if(req.method == "POST"_method) return handle_post_create(req, res);

    /* Preflight check */
    else return handle_cors_preflight(req, res);
  };

  auto post_dispatcher = [](const crow::request &req, crow::response &res, const uint64_t &id) -> void {
    /* List all */
    if(req.method == "GET"_method) return handle_post_read(req, res, id);

    /* Update post */
    else if(req.method == "POST"_method) return handle_post_update(req, res, id);

    /* Delete post */
    else if(req.method == "DELETE"_method) return handle_post_delete(req, res, id);
    
    /* Preflight check */
    else return handle_cors_preflight(req, res);
  };

  CROW_ROUTE(app, "/posts").methods("GET"_method, "POST"_method, "OPTIONS"_method)(posts_dispatcher);
  
  /* List page */
  CROW_ROUTE(app, "/posts/<int>").methods("GET"_method)(handle_post_list_page);

  /* Internal post methods */
  CROW_ROUTE(app, "/internal/post/<uint>").methods("GET"_method ,"POST"_method, "DELETE"_method, "OPTIONS"_method)(post_dispatcher);

  CROW_ROUTE(app, "/post/<string>").methods("GET"_method)(handle_post_read_url);

  //TODO: /post/<string>

  CROW_ROUTE(app, "/tag/<string>").methods("GET"_method)(handle_post_tag_list);
  CROW_ROUTE(app, "/tag/<string>/<uint>").methods("GET"_method)(handle_post_tag_list_page);

  if(c.server_multithreaded) {
    app.port(c.server_port).multithreaded().run();
  } else {
    app.port(c.server_port).run();
  }
}
