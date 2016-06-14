#include "auth.h"

#include <string>
#include <unordered_map>
#include <unordered_set>

namespace C3 {
  namespace Auth {
    std::unordered_set<std::string> authors;
    std::unordered_map<std::string, Session> sessions;

    void saveSession(const std::string &sid, const Session &s) {
      sessions[sid] = s;
    }

    Session getSession(const std::string &sid) {
      return sessions[sid]; // Creates if doesn't exists
    }

    bool isAuthor(const std::string &email) {
      return authors.count(email) > 0;
    }

    void addAuthor(const std::string &email) {
      authors.insert(email);
    }
  }
}
