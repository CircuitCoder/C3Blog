#include "auth.h"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <iostream>

#include "config.h"

namespace C3 {
  namespace Auth {
    std::unordered_set<std::string> authors;
    std::unordered_map<std::string, Session> sessions;

    void saveSession(const std::string &sid, const Session &s) {
      std::cout<<"Save: "<<sid<<std::endl;
      sessions[sid] = s;
    }

    Session getSession(const std::string &sid) {
      std::cout<<"Get: "<<sid<<std::endl;
      return sessions[sid]; // Creates if doesn't exists
    }

    bool isAuthor(const std::string &email) {
      return authors.count(email) > 0;
    }

    void addAuthor(const std::string &email) {
      authors.insert(email);
    }

    void setupAuthors(const Config &c) {
      authors.insert(c.user_authors.begin() ,c.user_authors.end());
    }
  }
}
