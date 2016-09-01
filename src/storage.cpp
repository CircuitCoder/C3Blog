#include "storage.h"
#include "util.h"
#include "mapper.h"

#include <iostream>
#include <cassert>
#include <cstdarg>
#include <chrono>
#include <leveldb/db.h>
#include <leveldb/cache.h>
#include <leveldb/write_batch.h>
#include <json/json.h>
#include <boost/filesystem.hpp>

#define INIT_DB(db) \
    leveldb::Options db##Opt; \
    db##Opt.create_if_missing = true; \
    db##Opt.comparator = &db##Cmp; \
    db##Opt.block_cache = cachePtr; \
    \
    leveldb::Status db##Status = leveldb::DB::Open(db##Opt, dir + "/" #db, &db##DB); \
    assert(db##Status.ok()); \

namespace C3 {

  extern Json::StreamWriterBuilder wbuilder;
  extern Json::CharReaderBuilder rbuilder;

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

  CommaSepComparator postCmp({ Limitor::Greater });
  CommaSepComparator commentCmp({ Limitor::Greater, Limitor::Less });
  CommaSepComparator entryCmp({ Limitor::Less, Limitor::Greater });
  CommaSepComparator indexCmp({ Limitor::Less, Limitor::Greater });

  leveldb::DB *postDB;
  leveldb::DB *indexDB;
  leveldb::DB *commentDB;
  leveldb::DB *entryDB;

  Post::Post(
      const std::string &uident,
      const std::string &url,
      const std::string &topic,
      const std::string &content,
      const std::vector<std::string> &tags,
      uint64_t post_time,
      uint64_t update_time) :
        uident(uident), url(url), topic(topic), content(content), tags(tags), post_time(post_time), update_time(update_time) {}

  Post::Post(const std::string &json) {
    Json::Value r;
    if(!parseFromString(rbuilder, json, &r)) throw ParseError;

    std::vector<std::string> tags(r["tags"].size());
    for(auto it = r["tags"].begin(); it != r["tags"].end(); ++it) {
      tags[it.index()] = it->asString();
    }

    uident = r["uident"].asString();
    url = r["url"].asString();
    topic = r["topic"].asString();
    content = r["content"].asString();
    this->tags = tags;
    post_time = r["post_time"].asUInt64();
    update_time = r["update_time"].asUInt64();

    // Not updated yet. For backward compatibility
    if(update_time == 0) update_time = post_time;
  }

  Post::Post(const std::string &json, const std::string &uident) :
        uident(uident), post_time(current_time()) {
    Json::Value r;
    if(!parseFromString(rbuilder, json, &r)) throw ParseError;

    std::vector<std::string> tags(r["tags"].size());
    for(auto it = r["tags"].begin(); it != r["tags"].end(); ++it) {
      tags[it.index()] = it->asString();
    }

    url = r["url"].asString();
    topic = r["topic"].asString();
    content = r["content"].asString();
    this->tags = tags;
    this->update_time = this->post_time;
  }

  std::string Post::to_json(void) const {
    Json::Value r;
    r["uident"] = uident;
    r["url"] = url;
    r["topic"] = topic;
    r["content"] = content;
    r["post_time"] = (Json::UInt64) post_time;
    r["update_time"] = (Json::UInt64) update_time;
    Json::Value v;
    for(auto e : tags) v.append(e);
    r["tags"] = v;

    return Json::writeString(wbuilder, r);
  }

  Comment::Comment(
      const std::string &uident,
      const std::string &content,
      uint64_t comment_time) :
        uident(uident), content(content), comment_time(comment_time) { }

  Comment::Comment(const std::string &json) {
    Json::Value r;
    if(!parseFromString(rbuilder, json, &r)) throw ParseError;

    uident = r["uident"].asString();
    content = r["content"].asString();
    comment_time = r["comment_time"].asUInt64();
  }

  Comment::Comment(const std::string &json, const std::string &uident) :
        uident(uident), comment_time(current_time()) {
    Json::Value r;
    if(!parseFromString(rbuilder, json, &r)) throw ParseError;

    content = r["content"].asString();
  }

  std::string Comment::to_json(void) const {
    Json::Value r;
    r["uident"] = uident;
    r["content"] = content;

    return Json::writeString(wbuilder, r);
  }

  User::User(
      UserType type,
      const std::string &id,
      const std::string &name,
      const std::string &email,
      const std::string &avatar) :
        type(type), id(id), name(name), email(email), avatar(avatar) { }

  User::User(const std::string &json) {
    Json::Value r;
    if(!parseFromString(rbuilder, json, &r)) throw ParseError;

    // Type
    std::string typestr = r["type"].asString();
    if(typestr == "google") type = uGoogle;
    else type = uUnknown;

    id = r["id"].asString();
    name = r["name"].asString();
    email = r["email"].asString();
    avatar = r["avatar"].asString();
  }

  std::string User::to_json(void) const {
    Json::Value r;
    if(type == uGoogle) r["type"] = "google";
    else r["type"] = "unknown";

    r["id"] = id;
    r["name"] = name;
    r["email"] = email;
    r["avatar"] = avatar;

    return Json::writeString(wbuilder, r);
  }

  CommaSepComparator::CommaSepComparator(std::initializer_list<Limitor> lims) :
     lims(lims) { }

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

      Limitor lim;
      if(i<this->lims.size()) lim = this->lims[i];
      else lim = Limitor::Less;

      if(lim == Limitor::Less) {
        if(ia[i] < ib[i]) return -1;
        else if(ia[i] > ib[i]) return 1;
      } else {
        if(ia[i] < ib[i]) return 1;
        else if(ia[i] > ib[i]) return -1;
      }
    }
  }

  const char* CommaSepComparator::Name() const { return "CommaSepComparator"; }

  void CommaSepComparator::FindShortestSeparator(std::string *, const leveldb::Slice &) const { }

  void CommaSepComparator::FindShortSuccessor(std::string *) const { }

  bool setup_storage(const std::string &dir, uint64_t cache) {
    // Ckeck if the folder exists

    boost::filesystem::path dbpath(dir);
    if(!boost::filesystem::exists(dbpath)) {
      std::cout<<"Storage: Creating database path: "<<dir<<std::endl;
      // Check for parent pathes

      for(auto p = dbpath.parent_path();; p = p.parent_path()) {
        if(p.empty()) break; // Valid
        if(!boost::filesystem::exists(p)) continue;
        if(boost::filesystem::is_directory(p)) break; // Valid
        else {
          std::cout<<"Storage: "<<p.native()<<" is not a directory."<<std::endl;
          return false;
        }
      }
      if(!boost::filesystem::create_directories(dbpath)) {
        std::cout<<"Storage: Failed to create database path"<<std::endl;
        return false;
      }
    } else if(!boost::filesystem::is_directory(dbpath)) {
      std::cout<<"Storage: "<<dir<<" is not a directory."<<std::endl;
      return false;
    }

    leveldb::Cache *cachePtr = cache > 0 ? leveldb::NewLRUCache(cache) : NULL;

    std::cout<<"Storage: Opening db at "<<dir<<std::endl;

    INIT_DB(post);
    INIT_DB(index);
    INIT_DB(comment);
    INIT_DB(entry);

    return true;
  }

  bool setup_url_map(void) {
    auto it = postDB->NewIterator(leveldb::ReadOptions());

    for(it->SeekToFirst(); it->Valid(); it->Next()) {
      try {
        Post p(it->value().ToString());
        add_url(p.url, p.post_time);
      } catch(MapperError e) {
        if(e == MapperError::DuplicatedUrl) return false;
        else throw;
      }
    }

    return true;
  }

  void stop_storage(void) {
    delete postDB;
    delete indexDB;
    delete commentDB;
    delete indexDB;
  }

  uint64_t add_post(const Post &post) {
    // Using milliseconds since Unix Epoch as post id
    uint64_t ts = post.post_time;
    
    //Assume that we can't submit two post at the same millisecond 
    leveldb::Status s = postDB->Put(leveldb::WriteOptions(), std::to_string(ts), post.to_json());
    if(s.ok()) return ts;
    else {
      throw s;
    }
  }

  Post get_post(const uint64_t &id) {
    return Post(get_post_str(id));
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

    // TODO: use r-value reference
    Post np = post;
    np.update_time = current_time();
    leveldb::Status s = postDB->Put(leveldb::WriteOptions(), std::to_string(id), np.to_json());
    if(!s.ok()) throw s;
  }
  
  void delete_post(const uint64_t &id) {
    leveldb::Status s = postDB->Delete(leveldb::WriteOptions(), std::to_string(id));
    if(!s.ok()) {
      if(s.IsNotFound()) throw NotFound;
      else throw s;
    }
  }

  std::list<Post> list_posts(int offset, int count, bool &hasNext) {
    leveldb::Iterator *it = postDB->NewIterator(leveldb::ReadOptions());
    it->SeekToFirst();

    std::list<Post> result;

    while(it->Valid() && offset-- > 0) it->Next();

    for(int i = 0; (count == -1 || i < count) && it->Valid(); ++i) {
      result.push_back(Post(it->value().ToString()));
      it->Next();
    }

    hasNext = it->Valid();

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

  std::list<uint64_t> list_posts_by_tag(const std::string &entry, int offset, int count, bool &hasNext) {
    leveldb::Iterator *it = entryDB->NewIterator(leveldb::ReadOptions());
    it->Seek(entry);

    std::list<uint64_t> result;
    while(it->Valid() && offset-- > 0) it->Next();

    for(int i = 0; (count == -1 || i < count) && it->Valid() && _entryEquals(it->key().ToString(), entry); ++i) {
      result.push_back(std::stoull(it->value().ToString()));
      it->Next();
    }

    hasNext = it->Valid() && _entryEquals(it->key().ToString(), entry);
    return result;
  }
}
