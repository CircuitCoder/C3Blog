#include <iostream>
#include <crow.h>
#include <signal.h>
#include <boost/program_options.hpp>

#include "storage.h"
#include "handler.h"
#include "middleware.h"
#include "util.h"
#include "config.h"
#include "auth.h"

using namespace C3;

void stop_handler(int s) {
  std::cout<<"Shutdown down..."<<std::endl;
  stop_storage();
}

namespace C3 {
  extern Json::StreamWriterBuilder wbuilder;
  extern Json::CharReaderBuilder rbuilder;
}

void setup_globals() {
  rbuilder["collectComments"] = false;
  wbuilder["commentStyle"] = "None";
  wbuilder["indentation"] = "";
}

void start_server(const Config &c) {
  crow::App<crow::CookieParser, Middleware> app;

  auto posts_dispatcher = [](const crow::request &req, crow::response &res) -> void {

    /* List all */
    if(req.method == "GET"_method) return handle_post_list(req, res);

    /* New post */
    else if(req.method == "POST"_method) return handle_post_create(req, res);
  };

  auto post_dispatcher = [](const crow::request &req, crow::response &res, const uint64_t &id) -> void {
    /* List all */
    if(req.method == "GET"_method) return handle_post_read(req, res, id);

    /* Update post */
    else if(req.method == "POST"_method) return handle_post_update(req, res, id);

    /* Delete post */
    else if(req.method == "DELETE"_method) return handle_post_delete(req, res, id);
  };

  CROW_ROUTE(app, "/posts").methods("GET"_method, "POST"_method)(posts_dispatcher);
  
  /* List page */
  CROW_ROUTE(app, "/posts/<int>").methods("GET"_method)(handle_post_list_page);

  /* Internal post methods */
  CROW_ROUTE(app, "/internal/post/<uint>").methods("GET"_method ,"POST"_method, "DELETE"_method)(post_dispatcher);

  CROW_ROUTE(app, "/post/<string>").methods("GET"_method)(handle_post_read_url);

  CROW_ROUTE(app, "/tag/<string>").methods("GET"_method)(handle_post_tag_list);
  CROW_ROUTE(app, "/tag/<string>/<uint>").methods("GET"_method)(handle_post_tag_list_page);

  CROW_ROUTE(app, "/account/login").methods("POST"_method)(handle_account_login);
  CROW_ROUTE(app, "/account/logout").methods("POST"_method)(handle_account_logout);

  CROW_ROUTE(app, "/feed").methods("GET"_method)(handle_feed);

  crow::logger::setLogLevel(crow::LogLevel::WARNING);

  if(c.server_multithreaded) {
    app.port(c.server_port).multithreaded().run();
  } else {
    app.port(c.server_port).run();
  }
}

int main(int argc, char** argv) {
  namespace po = boost::program_options;
  po::options_description desc("CLI Options");
  desc.add_options()
    ("help", "print help message")
    ("check,C", "Perform storage check before server startup")
    ("check-authors", "Perform author check before server startup");
  po::variables_map opts;
  po::store(po::parse_command_line(argc, argv, desc), opts);
  po::notify(opts);

  if(opts.count("help")) {
    std::cout<<desc<<std::endl;
    return 0;
  }

  bool flag_check = opts.count("check");
  bool flag_check_authors = flag_check || opts.count("check-authors");

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

  if(!setup_storage(c.db_path, c.db_cache)) {
    std::cout<<"Failed to initialize storage. Aborting."<<std::endl;
    return -1;
  }

  setup_globals();
  setup_handlers(c);
  setup_middleware(c);
  setup_url_map();
  Auth::setupAuthors(c);
  Feed::setup(c);

  bool validFlag = true;

  if(flag_check_authors) {
    if(!check_authors()) {
      std::cout<<"An unexpected error occured during the author check."<<std::endl;
      validFlag = false;
    }
  }

  if(!validFlag) {
    stop_storage();
    return 1;
  }

  std::cout<<"Starting server..."<<std::endl;
  start_server(c);
}
