#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <leveldb/db.h>
#include <leveldb/comparator.h>

#include "util.h"

namespace C3 {
  enum StorageExcept {
    NotFound = 0,
    ParseError = 1,
    IDMismatch = 2
  };

  struct Post {
    std::string uident;
    std::string url;
    std::string topic;
    std::string content;
    std::vector<std::string> tags;

    uint64_t post_time;

    Post(
        const std::string &uident,
        const std::string &url,
        const std::string &topic,
        const std::string &content,
        const std::vector<std::string> &tags,
        uint64_t post_time);

    Post(const std::string &json);

    Post(const std::string &json, const std::string &uident);

    std::string to_json(void) const;
  };

  struct Comment {
    std::string uident; // type,id
    std::string content;

    uint64_t comment_time;

    Comment(
        const std::string &uident,
        const std::string &content,
        uint64_t comment_time);

    Comment(const std::string &json);

    Comment(const std::string &json, const std::string &uident);

    std::string to_json(void) const;
  };

  struct User {
    enum UserType {
      uGoogle, // Currently only support google login
      uUnknown
    };

    UserType type;
    std::string id;

    std::string name;
    std::string email;
    std::string avatar;

    User(
        UserType type,
        const std::string &id,
        const std::string &name,
        const std::string &email,
        const std::string &avatar);

    User(const std::string &json);

    std::string to_json(void) const;

    std::string getKey(void) const {
      if(type == uGoogle)
        return "google," + id;
      else
        return "unknown," + id;
    }
  };

  class CommaSepComparator : public leveldb::Comparator {

  public:
    int Compare(const leveldb::Slice &a, const leveldb::Slice &b) const;
    const char* Name() const;
    void FindShortestSeparator(std::string *, const leveldb::Slice &) const;
    void FindShortSuccessor(std::string *) const;
  };

  typedef struct Post Post;
  typedef struct Comment Comment;

  bool setup_storage(const std::string &dir);
  void stop_storage(void);

  /* Posts */
  uint64_t add_post(const Post &post);
  void update_post(const uint64_t &id, const Post &post);
  void delete_post(const uint64_t &id);
  Post get_post(const uint64_t &id);
  std::string get_post_str(const uint64_t &id);
  std::list<Post> list_posts(int offset, int count);

  /* Comments */
  uint64_t add_comment(const Comment &comment);
  std::vector<Comment> get_comment(uint64_t post_id);
  void delete_comment(uint64_t post_id, uint64_t comment_id);

  /* Entries */
  void remove_entries(const uint64_t &id, const std::list<std::string> &list);
  void add_entries(const uint64_t &id, const std::list<std::string> &list);
  void add_remove_entries(const uint64_t &id, const std::list<std::string> &added, const std::list<std::string> &removed);
  std::list<uint64_t> list_posts_by_tag(const std::string &entry, int offset, int count);

  //TODO: Indexes
};
