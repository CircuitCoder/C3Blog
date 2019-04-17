#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <leveldb/db.h>
#include <leveldb/comparator.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

#include "util.h"

namespace rj = rapidjson;

namespace C3 {
  enum class StorageExcept {
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
    uint64_t update_time;

    Post(
        const std::string &uident,
        const std::string &url,
        const std::string &topic,
        const std::string &content,
        const std::vector<std::string> &tags,
        uint64_t post_time,
        uint64_t update_time);

    Post(const std::string &json);

    Post(const std::string &json, const std::string &uident);

    std::string to_json(void) const;

    void write_json(rj::Writer<rj::StringBuffer> &, bool = true) const;
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

    void write_json(rj::Writer<rj::StringBuffer> &, bool = true) const;
  };

  struct User {
    enum class UserType {
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

    void write_json(rj::Writer<rj::StringBuffer> &, bool = true) const;

    std::string getKey(void) const {
      if(type == UserType::uGoogle)
        return "google," + id;
      else
        return "unknown," + id;
    }
  };

  enum class Limitor {
    Less, Greater
  };

  class CommaSepComparator : public leveldb::Comparator {
  private:
    std::vector<Limitor> lims;

  public:
    CommaSepComparator(std::initializer_list<Limitor> lims);
    int Compare(const leveldb::Slice &a, const leveldb::Slice &b) const;
    const char* Name() const;
    void FindShortestSeparator(std::string *, const leveldb::Slice &) const;
    void FindShortSuccessor(std::string *) const;
  };

  typedef struct Post Post;
  typedef struct Comment Comment;
  typedef struct User User;

  bool setup_storage(const std::string &dir, uint64_t cache);
  bool setup_url_map(void);
  void stop_storage(void);
  bool check_authors(void);

  /* Posts */
  uint64_t add_post(const Post &post);
  void update_post(const uint64_t &id, const Post &post);
  void delete_post(const uint64_t &id);
  Post get_post(const uint64_t &id);
  std::string get_post_str(const uint64_t &id);
  std::vector<Post> list_posts(int offset, int count, bool &hasNext, uint64_t &total);

  /* Comments */
  uint64_t add_comment(const Comment &comment);
  std::vector<Comment> get_comment(uint64_t post_id);
  void delete_comment(uint64_t post_id, uint64_t comment_id);

  /* Entries */
  void remove_entries(const uint64_t &id, const std::vector<std::string> &list);
  void add_entries(const uint64_t &id, const std::vector<std::string> &list);
  void add_remove_entries(const uint64_t &id, const std::vector<std::string> &added, const std::vector<std::string> &removed);
  std::vector<uint64_t> list_posts_by_tag(const std::string &entry, int offset, int count, bool &hasNext, uint64_t &total);

  /* Users */
  bool update_user(const User &user);
  User get_user(const std::string &uident);

  /* Index */
  void set_indexes(uint64_t post, const std::unordered_map<std::string, std::vector<std::pair<uint32_t, bool>>> &indexes);
  void clear_indexes(uint64_t post);
  std::unordered_map<uint64_t, std::vector<std::pair<uint32_t, bool>>> query_indexes(const std::string &str);
};
