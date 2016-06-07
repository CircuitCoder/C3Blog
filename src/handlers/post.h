#pragma once

#include <list>
#include <sstream>
#include <crow.h>
#include <json/json.h>

#include "storage.h"

const int PPP = 10; // Post per page

Json::FastWriter writer;
Json::Reader reader;

namespace C3 {
  void handle_post_list(const crow::request &req, crow::response &res);
  void handle_post_list_page(const crow::request &req, crow::response &res, int page);
  void handle_post_tag_list(const crow::request &req, crow::response &res, const std::string &tag);
  void handle_post_tag_list_page(const crow::request &req, crow::response &res, const std::string &tag, int page);
  void handle_post_read(const crow::request &req, crow::response &res, uint64_t id);
  void handle_post_create(const crow::request &req, crow::response &res);
  void handle_post_update(const crow::request &req, crow::response &res, uint64_t id);
  void handle_post_delete(const crow::request &req, crow::response &res, uint64_t id);

  void handle_post_list(const crow::request &req, crow::response &res) {
    return handle_post_list_page(req, res, 1);
  }

  void handle_post_list_page(const crow::request &req, crow::response &res, int page) {
    int offset = (page-1) * PPP;
    int count = PPP;

    std::list<Post> p = list_posts(offset, count);
    // TODO: total count
    Json::Value v(Json::arrayValue);
    for(auto i = p.begin(); i != p.end(); ++i) {
      Json::Value e;
      e["url"] = i->url;
      e["topic"] = i->topic;
      e["post_time"] = (Json::UInt64) i->post_time;

      v.append(e);
    }

    res.end(writer.write(v));
  }

  void handle_post_tag_list(const crow::request &req, crow::response &res, const std::string &tag) {
    return handle_post_tag_list_page(req, res, tag, 1);
  }

  void handle_post_tag_list_page(const crow::request &req, crow::response &res, const std::string &tag, int page) {
    int offset = (page-1) * PPP;
    int count = PPP;

    std::list<uint64_t> ids = list_posts_by_tag(tag, offset, count);

    Json::Value v(Json::arrayValue);
    for(auto i = ids.begin(); i != ids.end(); ++i) {
      Post p = get_post(*i);
      Json::Value e;
      e["url"] = p.url;
      e["topic"] = p.topic;
      e["post_time"] = (Json::UInt64) p.post_time;

      v.append(e);
    }

    res.end(writer.write(v));
  }

  void handle_post_read(const crow::request &req, crow::response &res, uint64_t id) {
    try {
      std::string cont = get_post_str(id);
      res.end(cont);
      return;
    } catch(StorageExcept &e) {
      if(e == NotFound) {
        res.code = 404;
        res.end("404 Not Found");
      } else {
        res.code = 500;
        res.end("500 Internal Error");
      }
    } catch(...) {
      res.code = 500;
      res.end("500 Internal Error");
    }
  }

  void handle_post_create(const crow::request &req, crow::response &res) {
    try {
      Post p(req.body, "unknown,dummy");
      sort(p.tags.begin(), p.tags.end());
      uint64_t id = add_post(p);

      //TODO: template
      std::list<std::string> tags(p.tags.begin(), p.tags.end());

      //TODO: batch
      add_entries(id, tags);

      Json::Value v;
      v["id"] = (Json::UInt64) id;
      res.end(writer.write(v));
    } catch(...) {
      res.code = 500;
      res.end("500 Internal Error");
    }
  }

  void handle_post_update(const crow::request &req, crow::response &res, uint64_t id) {
    try {
      Post original = get_post(id);
      Post current(req.body);

      // Tags
      sort(current.tags.begin(), current.tags.end());

      std::list<std::string> removed;
      std::list<std::string> added;

      auto oriIt = original.tags.begin();
      auto curIt = current.tags.begin();

      while(true) {
        if(oriIt == original.tags.end()) {
          while(curIt != current.tags.end())
            added.push_back(*curIt++);
          break;
        } else if(curIt == current.tags.end()) {
          while(oriIt != original.tags.end())
            removed.push_back(*oriIt++);
          break;
        } else {
          if(*curIt < *oriIt)
            added.push_back(*curIt++);
          else if(*curIt > *oriIt)
            removed.push_back(*oriIt++);
          else {
            ++curIt;
            ++oriIt;
          }
        }
      }

      //TODO: url

      //TODO: batch
      add_remove_entries(id, added, removed);
      update_post(id, Post(req.body));

      res.end("{\"ok\":0}");
    } catch(StorageExcept &e) {
      if(e == IDMismatch) {
        res.code = 400;
        res.end("400 Bad Request");
      } else {
        res.code = 500;
        res.end("500 Internal Error");
      }
    } catch(...) {
      res.code = 500;
      res.end("500 Internal Error");
    }
  }

  void handle_post_delete(const crow::request &req, crow::response &res, uint64_t id) {
    try {
      Post p = get_post(id);

      //TODO: batch
      std::list<std::string> tags(p.tags.begin(), p.tags.end());
      remove_entries(id, tags);
      delete_post(id);

      res.end("{\"ok\":0}");
    } catch(StorageExcept &e) {
      if(e == NotFound) {
        res.code = 404;
        res.end("404 Not Found");
      } else {
        res.code = 500;
        res.end("500 Internal Error");
      }
    } catch(...) {
      res.code = 500;
      res.end("500 Internal Error");
    }
  }
}
