#pragma once

#include <string>

#include "config.h"
namespace C3 {
  namespace Auth {
    enum class AuthError {
      NotSignedIn
    };

    typedef struct Session {
      bool signedIn;
      std::string uident;
      std::string token;
      bool isAuthor;
    } Session;

    void saveSession(const std::string &sid, const Session &s);
    Session getSession(const std::string &sid);

    bool isAuthor(const std::string &email);
    void addAuthor(const std::string &email);
    void setupAuthors(const Config &c);
  }
}
