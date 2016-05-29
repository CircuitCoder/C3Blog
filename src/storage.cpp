#include "storage.h"
#include "util.h"

#include <iostream>
#include <cassert>
#include <chrono>
#include <leveldb/db.h>
#include <leveldb/write_batch.h>
#include <json/json.h>

namespace C3 {

  bool _entryEquals(const std::string &entry, const std::string &key) {
    auto eit = entry.begin();
    auto kit = key.begin();

    while(kit != key.end()) {
      if(eit == entry.end()) return false;
      else if(*eit != *kit) return false;
      ++kit;
      ++eit;
    }

    if(eit == entry.end() || *eit == ',') return true;
    else return false;
  }


  Randomizer _rand;
  CommaSepComparator comp;

  leveldb::DB *postDB;
  leveldb::DB *indexDB;
  leveldb::DB *commentDB;
  leveldb::DB *entryDB;

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

  int CommaSepComparator::Compare(const leveldb::Slice &a, const leveldb::Slice &b) const {
    std::string sa = a.ToString();
    std::string sb = b.ToString();

    std::vector<std::string> ia = split(sa, ',');
    std::vector<std::string> ib = split(sb, ',');

    for(int i = 0; ; ++i) {
      if(i == ia.size()) {
        if(i == ib.size()) return 0;
        else return -1;
      } else if(i == ib.size()) return 1;

      if(ia[i] < ib[i]) return -1;
      else if(ia[i] > ib[i]) return 1;
    }
  }

  const char* CommaSepComparator::Name() const { return "CommaSepComparator"; }

  void CommaSepComparator::FindShortestSeparator(std::string *, const leveldb::Slice &) const { }

  void CommaSepComparator::FindShortSuccessor(std::string *) const { }

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

    // TODO: cache
    std::cout<<"Opening/Creating db at "<<dir<<std::endl;

    leveldb::Options dbOpt;
    dbOpt.create_if_missing = true;
    dbOpt.comparator = &comp;

    leveldb::Status postStatus = leveldb::DB::Open(dbOpt, dir + "/post", &postDB);
    assert(postStatus.ok());

    leveldb::Status indexStatus = leveldb::DB::Open(dbOpt, dir + "/index", &indexDB);
    assert(indexStatus.ok());

    leveldb::Status commentStatus = leveldb::DB::Open(dbOpt, dir + "/comment", &commentDB);
    assert(commentStatus.ok());

    leveldb::Status entryStatus = leveldb::DB::Open(dbOpt, dir + "/entry", &entryDB);
    assert(entryStatus.ok());
  }

  void stop_storage(void) {
    delete postDB;
    delete indexDB;
    delete commentDB;
    delete indexDB;
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

  std::list<Post> list_posts(int offset, int count) {
    leveldb::Iterator *it = postDB->NewIterator(leveldb::ReadOptions());
    it->SeekToFirst();

    std::list<Post> result;

    while(it->Valid() && offset-- > 0) it->Next();

    for(int i = 0; i < count && it->Valid(); ++i) {
      result.push_back(json_to_post(it->value().ToString()));
      it->Next();
    }

    return result;
  }

  /* Entries */
  void _generate_remove_entries(const uint64_t &id, const std::list<std::string> &list, leveldb::WriteBatch &batch) {
    for(auto it = list.begin(); it != list.end(); ++it)
      batch.Delete(*it + "," + std::to_string(id));
  }

  void _generate_add_entries(const uint64_t &id, const std::list<std::string> &list, leveldb::WriteBatch &batch) {
    for(auto it = list.begin(); it != list.end(); ++it)
      batch.Put(*it + "," + std::to_string(id), std::to_string(id));
  }

  void remove_entries(const uint64_t &id, const std::list<std::string> &list) {
    leveldb::WriteBatch batch;
    _generate_remove_entries(id, list, batch);
    entryDB->Write(leveldb::WriteOptions(), &batch);
  }

  void add_entries(const uint64_t &id, const std::list<std::string> &list) {
    leveldb::WriteBatch batch;
    _generate_add_entries(id, list, batch);
    entryDB->Write(leveldb::WriteOptions(), &batch);
  }

  void add_remove_entries(const uint64_t &id, const std::list<std::string> &add, const std::list<std::string> &remove) {
    leveldb::WriteBatch batch;
    _generate_add_entries(id, add, batch);
    _generate_remove_entries(id, remove, batch);
    /* TODO: status */
    entryDB->Write(leveldb::WriteOptions(), &batch);
  }

  std::list<uint64_t> list_posts_by_tag(const std::string &entry, int offset, int count) {
    leveldb::Iterator *it = entryDB->NewIterator(leveldb::ReadOptions());
    it->Seek(entry);

    std::list<uint64_t> result;
    while(it->Valid() && offset-- > 0) it->Next();

    for(int i = 0; i < count && it->Valid() && _entryEquals(it->key().ToString(), entry); ++i) {
      result.push_back(std::stoull(it->value().ToString()));
      it->Next();
    }
    return result;
  }
}
