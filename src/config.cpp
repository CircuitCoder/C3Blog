#include <yaml-cpp/yaml.h>
#include <cassert>
#include <string>
#include <iostream>

#include "config.h"

#define WARN_BAD_FORMAT(ident,expected) std::cout<<"Config: Bad format. "<<ident<<" should be "<<expected<<"."<<std::endl;
#define READ_CONFIG(ident,source,target,type,expected) \
  try { \
    this->target = config source.as<type>(); \
  } catch(YAML::BadConversion e) { \
    WARN_BAD_FORMAT(ident, expected); \
    return false; \
  }

#define READ_SEQUENCE(ident,source,target,type,expected) \
  try { \
    if(config source.IsSequence()) { \
      for(auto it = config source.begin(); it != config source.end(); ++it) { \
        this->target.push_back(it->as<type>()); \
      } \
    } else { \
      WARN_BAD_FORMAT(ident, "a list containing expected"); \
      return false; \
    } \
  } catch(YAML::BadConversion e) { \
    WARN_BAD_FORMAT(ident, "a list containing expected"); \
    return false; \
  } \


namespace C3 {
  Config::Config() { }

  bool Config::read(const std::string &path) {
    try {
      YAML::Node config;
      try {
        config = YAML::LoadFile("config.yml");
      } catch(YAML::BadFile e) {
        std::cout<<"Config: Bad file. Please check if the config file exists."<<std::endl;
        return false;
      }

      READ_CONFIG("app.title", ["app"]["title"], app_title, std::string, "a string");
      READ_CONFIG("app.url", ["app"]["url"], app_url, std::string, "a string");
      READ_CONFIG("app.feed_length", ["app"]["feed_length"], app_feedLength, uint16_t, "a integer");
      READ_CONFIG("app.page_length", ["app"]["page_length"], app_pageLength, uint16_t, "a integer");

      READ_CONFIG("server.port", ["server"]["port"], server_port, uint16_t, "an integer");
      READ_CONFIG("server.multithreaded", ["server"]["multithreaded"], server_multithreaded, bool, "a boolean");

      READ_CONFIG("db.path", ["db"]["path"], db_path, std::string, "a string");
      READ_CONFIG("db.cache", ["db"]["cache"], db_cache, uint64_t, "an integer");

      READ_SEQUENCE("security.origins", ["security"]["origins"], security_origins, std::string, "strings");

      READ_SEQUENCE("user.authors", ["user"]["authors"], user_authors, std::string, "strings");

    } catch(YAML::Exception e) {
      std::cout<<"Config: Unknown exception:"<<std::endl;
      std::cout<<e.what()<<std::endl;
      return false;
    }
    return true;
  }
}
