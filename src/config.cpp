#include <yaml-cpp/yaml.h>
#include <string>
#include <iostream>

#include "config.h"

#define WARN_BAD_FORMAT(ident,expected) std::cout<<"Config: Bad format. "<<ident<<" should be "<<expected<<"."<<std::endl;

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

      try {
        this->server_port = config["server"]["port"].as<uint16_t>();
      } catch(YAML::BadConversion e) {
        WARN_BAD_FORMAT("server.port", "an integer");
        return false;
      }

      try {
        this->server_multithreaded = config["server"]["multithreaded"].as<bool>();
      } catch(YAML::BadConversion e) {
        WARN_BAD_FORMAT("server.multithreaded", "a boolean");
        return false;
      }

      try {
        this->db_path = config["db"]["path"].as<std::string>();
      } catch(YAML::BadConversion e) {
        WARN_BAD_FORMAT("db.path", "a string");
        return false;
      }
    } catch(YAML::Exception e) {
      std::cout<<"Config: Unknown exception:"<<std::endl;
      std::cout<<e.what()<<std::endl;
      return false;
    }
    return true;
  }
}
