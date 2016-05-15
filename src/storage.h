#pragma once

#include <string>
#include <vector>

namespace C3 {
  enum ReadExcept {
    NotFound = 0,
    ParseError = 1
  };

  enum WriteExcept {
    IDMismatch = 0
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

  uint64_t add_post(const Post &post);
  void update_post(const uint64_t &id, const Post &post);
  void delete_post(const uint64_t &id);
  Post get_post(const uint64_t &id);
  std::string get_post_str(const uint64_t &id);
  std::vector<std::pair<uint64_t, Post>> list_posts(std::vector<std::string> tags);

  uint64_t add_comment(const Comment &comment);
  std::vector<Comment> get_comment(uint64_t post_id);
  bool delete_comment(uint64_t post_id, uint64_t comment_id);

  //TODO: Indexes
};
