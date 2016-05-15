#include "storage.h"
#include "util.h"

#include <iostream>
#include <cassert>
#include <chrono>
#include <leveldb/db.h>
#include <json/json.h>

namespace C3 {
  Randomizer _rand;
  leveldb::DB *postDB;

  Json::Reader reader;
  Json::FastWriter writer;

  Post::Post(
      std::string url,
      std::string topic,
      std::string source,
      std::string content,
      std::vector<std::string> tags,
      uint64_t post_time) :
        url(url), topic(topic), source(source), content(content), tags(tags), post_time(post_time) {}

  Comment::Comment(
      std::string email,
      std::string content,
      uint64_t comment_time) :
        email(email), content(content), comment_time(comment_time) { }


  std::string post_to_json(const Post& p) {
    Json::Value r;
    r["url"] = p.url;
    r["topic"] = p.topic;
    r["source"] = p.source;
    r["content"] = p.content;
    r["post_time"] = p.post_time;
    Json::Value v;
    for(auto &&e : p.tags) v.append(e);
    r["tags"] = v;

    return writer.write(r);
  }

  std::string comment_to_json(const Comment& c) {
    Json::Value r;
    r["email"] = c.email;
    r["content"] = c.content;

    return writer.write(r);
  }

  Post json_to_post(const std::string& str) {
    Json::Value r;
    if(!reader.parse(str, r)) throw ParseError;

    std::vector<std::string> tags(r["tags"].size());
    for(auto it = r["tags"].begin(); it != r["tags"].end(); ++it) {
      tags[it.index()] = it->asString();
    }

    Post p(
        r["url"].asString(),
        r["topic"].asString(),
        r["source"].asString(),
        r["content"].asString(),
        tags,
        r["post_time"].asUInt64());
    return p;
  }

  Post json_to_new_post(const std::string &str) {
    Json::Value r;
    if(!reader.parse(str, r)) throw ParseError;

    std::vector<std::string> tags(r["tags"].size());
    for(auto it = r["tags"].begin(); it != r["tags"].end(); ++it) {
      tags[it.index()] = it->asString();
    }

    Post p(
        r["url"].asString(),
        r["topic"].asString(),
        r["source"].asString(),
        r["content"].asString(),
        tags,
        current_time());
    return p;
  }

  Comment json_to_comment(const std::string &str) {
    Json::Value r;
    if(!reader.parse(str, r)) throw ParseError;

    Comment c(
        r["email"].asString(),
        r["content"].asString(),
        r["comment_time"].asUInt64());
    return c;
  }

  Comment json_to_new_comment(const std::string &str) {
    Json::Value r;
    if(!reader.parse(str, r)) throw ParseError;

    Comment c(
        r["email"].asString(),
        r["content"].asString(),
        current_time());
    return c;
  }

  void setup_storage(const std::string &dir) {

    // Post DB
    // TODO: cache
    std::cout<<"Opening/Creating db at "<<dir<<std::endl;
    leveldb::Options postOpt;
    postOpt.create_if_missing = true;
    leveldb::Status postStatus = leveldb::DB::Open(postOpt, dir + "/post", &postDB);
    assert(postStatus.ok());
  }

  void stop_storage(void) {
    delete postDB;
  }

  uint64_t add_post(const Post &post) {
    // Using nanoseconds since Unix Epoch as post id
    uint64_t ts = post.post_time;
    
    //Assume that we can't submit two post at the same nanosecond
    //TODO: upate tags and indexes
    leveldb::Status s = postDB->Put(leveldb::WriteOptions(), std::to_string(ts), post_to_json(post));
    if(s.ok()) return ts;
    else {
      throw s;
    }
  }

  Post get_post(const uint64_t &id) {
    return json_to_post(get_post_str(id));
  }

  std::string get_post_str(const uint64_t &id) {
    std::string v;
    leveldb::Status s = postDB->Get(leveldb::ReadOptions(), std::to_string(id), &v);
    if(s.ok()) return v;
    else {
      if(s.IsNotFound()) throw NotFound;
      else throw s;
    }
  }

  void update_post(const uint64_t &id, const Post &post) {
    if(!(post.post_time == id)) throw IDMismatch;
    //TODO: upate tags and indexes
    leveldb::Status s = postDB->Put(leveldb::WriteOptions(), std::to_string(id), post_to_json(post));
    if(!s.ok()) throw s;
  }
  
  void delete_post(const uint64_t &id) {
    leveldb::Status s = postDB->Delete(leveldb::WriteOptions(), std::to_string(id));
    if(!s.ok()) {
      if(s.IsNotFound()) throw NotFound;
      else throw s;
    }
  }
}
