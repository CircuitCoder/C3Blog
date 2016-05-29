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
    std::string url;
    std::string topic;
    std::string source;
    std::string content;
    std::vector<std::string> tags;

    uint64_t post_time;

    Post(
        std::string url,
        std::string topic,
        std::string source,
        std::string content,
        std::vector<std::string> tags,
        uint64_t post_time);
  };

  struct Comment {
    std::string email;
    std::string content;

    uint64_t comment_time;

    Comment(
        std::string email,
        std::string content,
        uint64_t comment_time);
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

  std::string post_to_json(const Post& p);
  std::string comment_to_json(const Comment& c);
  Post json_to_post(const std::string& str);
  Post json_to_new_post(const std::string& str);
  Comment json_to_comment(const std::string &str);
  Comment json_to_new_comment(const std::string &str);

  void setup_storage(const std::string &dir);
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
