#pragma once

#include <list>
#include <sstream>
#include <crow.h>
#include <json/json.h>

#include "storage.h"
#include "mapper.h"
#include "middleware.h"
#include "config.h"
#include "../feed.h"

namespace C3 {

  int post_per_page = 10; // Post per page
  extern Json::StreamWriterBuilder wbuilder;

  void setup_post_handler(const Config &c) {
    post_per_page = c.app_pageLength;
  }

  void handle_post_list(const crow::request &req, crow::response &res);
  void handle_post_list_page(const crow::request &req, crow::response &res, int page);
  void handle_post_tag_list(const crow::request &req, crow::response &res, const std::string &tag);
  void handle_post_tag_list_page(const crow::request &req, crow::response &res, const std::string &tag, int page);
  void handle_post_read(const crow::request &req, crow::response &res, uint64_t id);
  void handle_post_read_url(const crow::request &req, crow::response &res, const std::string &url);
  void handle_post_create(const crow::request &req, crow::response &res);
  void handle_post_update(const crow::request &req, crow::response &res, uint64_t id);
  void handle_post_delete(const crow::request &req, crow::response &res, uint64_t id);

  void handle_post_list(const crow::request &req, crow::response &res) {
    return handle_post_list_page(req, res, 1);
  }

  void handle_post_list_page(const crow::request &req, crow::response &res, int page) {
    int offset = (page-1) * post_per_page;
    int count = post_per_page;

    bool hasNext;

    std::list<Post> p = list_posts(offset, count, hasNext);
    // TODO: total count
    Json::Value posts(Json::arrayValue);
    for(auto i = p.begin(); i != p.end(); ++i) {
      Json::Value e;
      e["url"] = i->url;
      e["topic"] = i->topic;
      e["post_time"] = (Json::UInt64) i->post_time;

      posts.append(e);
    }

    Json::Value v;
    v["posts"] = posts;
    v["hasNext"] = hasNext;

    res.end(Json::writeString(wbuilder, v));
  }

  void handle_post_tag_list(const crow::request &req, crow::response &res, const std::string &tag) {
    return handle_post_tag_list_page(req, res, tag, 1);
  }

  void handle_post_tag_list_page(const crow::request &req, crow::response &res, const std::string &tag, int page) {
    int offset = (page-1) * post_per_page;
    int count = post_per_page;

    bool hasNext;

    std::list<uint64_t> ids = list_posts_by_tag(URLEncoding::url_decode(tag), offset, count, hasNext);

    Json::Value posts(Json::arrayValue);
    for(auto i = ids.begin(); i != ids.end(); ++i) {
      Post p = get_post(*i);
      Json::Value e;
      e["url"] = p.url;
      e["topic"] = p.topic;
      e["post_time"] = (Json::UInt64) p.post_time;

      posts.append(e);
    }

    Json::Value v;
    v["posts"] = posts;
    v["hasNext"] = hasNext;

    res.end(Json::writeString(wbuilder, v));
  }

  void handle_post_read(const crow::request &req, crow::response &res, uint64_t id) {
    try {
      Post p  = get_post(id);
      Json::Value v = p.to_json_obj();

      try {
        User u = get_user(p.uident);
        v["user"] = u.to_json_obj();
      } catch(StorageExcept e) {
        CROW_LOG_WARNING << "No such user: " << p.uident;
      }

      res.end(Json::writeString(wbuilder, v));
      return;
    } catch(StorageExcept &e) {
      if(e == StorageExcept::NotFound) {
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

  void handle_post_read_url(const crow::request &req, crow::response &res, const std::string &url) {
    uint64_t id;
    try {
      id = query_url(URLEncoding::url_decode(url));
    } catch(MapperError e) {
      if(e == MapperError::UrlNotFound) {
        res.code = 404;
        res.end("404 Not Found");
      }
    }

    return handle_post_read(req, res, id);
  }

  void handle_post_create(const crow::request &req, crow::response &res) {
    try {
      // Context

      context * ctx = (context *) req.middleware_context;
      Middleware::context &cookieCtx = ctx->get<Middleware>();
      
      if(!(cookieCtx.session.signedIn && cookieCtx.session.isAuthor)) {
        res.code = 403;
        res.end("403 Forbidden");
        return;
      }

      CROW_LOG_INFO << "New post by: " << cookieCtx.session.uident;

      Post p(req.body, cookieCtx.session.uident);

      if(has_url(p.url)) {
        res.code = 200;
        res.end("{ error: 'dulicatedUrl' }");
        return;
      }

      sort(p.tags.begin(), p.tags.end());
      uint64_t id = add_post(p);

      //TODO: thread safety
      add_url(p.url, id);

      //TODO: template
      std::list<std::string> tags(p.tags.begin(), p.tags.end());

      //TODO: batch
      add_entries(id, tags);

      Feed::invalidate();

      Json::Value v;
      v["id"] = (Json::UInt64) id;
      res.end(Json::writeString(wbuilder, v));
    } catch(...) {
      res.code = 500;
      res.end("500 Internal Error");
    }
  }

  void handle_post_update(const crow::request &req, crow::response &res, uint64_t id) {

    context * ctx = (context *) req.middleware_context;
    Middleware::context &cookieCtx = ctx->get<Middleware>();
    
    if(!(cookieCtx.session.signedIn && cookieCtx.session.isAuthor)) {
      res.code = 403;
      res.end("403 Forbidden");
      return;
    }

    try {
      Post original = get_post(id);
      Post current(req.body);
      current.uident = original.uident;

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

      if(original.url != current.url)
        rename_url(original.url, current.url, id);
      //TODO: handle validation

      //TODO: batch
      add_remove_entries(id, added, removed);
      update_post(id, current);

      Feed::invalidate();

      res.end("{\"ok\":0}");
    } catch(StorageExcept &e) {
      if(e == StorageExcept::IDMismatch) {
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

    context * ctx = (context *) req.middleware_context;
    Middleware::context &cookieCtx = ctx->get<Middleware>();
    
    if(!(cookieCtx.session.signedIn && cookieCtx.session.isAuthor)) {
      res.code = 403;
      res.end("403 Forbidden");
      return;
    }

    try {
      Post p = get_post(id);

      remove_url(p.url);
      //TODO: batch
      std::list<std::string> tags(p.tags.begin(), p.tags.end());
      remove_entries(id, tags);
      delete_post(id);

      Feed::invalidate();

      res.end("{\"ok\":0}");
    } catch(StorageExcept &e) {
      if(e == StorageExcept::NotFound) {
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
