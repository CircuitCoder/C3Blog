#include "storage.h"
#include "util.h"
#include "mapper.h"
#include "saxreader.h"

#include <iostream>
#include <sstream>
#include <memory>
#include <cassert>
#include <cstdarg>
#include <charconv>
#include <chrono>
#include <leveldb/db.h>
#include <leveldb/cache.h>
#include <leveldb/write_batch.h>
#include <boost/filesystem.hpp>
#include <rapidjson/document.h>
#include <rapidjson/reader.h>
#include <rapidjson/stream.h>
#include <rapidjson/encodings.h>

#define INIT_DB(db) \
    leveldb::Options db##Opt; \
    db##Opt.create_if_missing = true; \
    db##Opt.comparator = &db##Cmp; \
    db##Opt.block_cache = cachePtr; \
    \
    leveldb::Status db##Status = leveldb::DB::Open(db##Opt, dir + "/" #db, &db##DB); \
    assert(db##Status.ok()); \

namespace C3 {
  template<typename Encoding>
  struct GenericBoundedStringStream {
    typedef typename Encoding::Ch Ch;

    GenericBoundedStringStream(const Ch *src, const size_t size) : src_(src), head_(src), left(size) {}

    Ch Peek() const {
      if(left == 0) return '\0';
      return *src_;
    }

    Ch Take() {
      if(left == 0) return '\0';
      --left;
      return *src_++;
    }

    size_t Tell() {
      return static_cast<size_t>(src_ - head_);
    }

    Ch* PutBegin() { throw "Not implemented"; return 0; }
    void Put([[maybe_unused]] Ch c) { throw "Not implemented"; }
    void Flush() { throw "Not implemented"; }
    size_t PutEnd([[maybe_unused]] Ch* begin) { throw "Not implemented"; return 0; };

    const Ch* src_;
    const Ch* head_;
    size_t left;
  };

  typedef GenericBoundedStringStream<rj::UTF8<>> BoundedStringStream;

  bool _entryEquals(const std::string_view &entry, const std::string_view &key) {
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

  bool _entryEquals(const std::string &entry, const std::string_view &key) {
    const std::string_view view(entry);
    return _entryEquals(view, key);
  }

  bool _entryEquals(const leveldb::Slice &entry, const std::string_view &key) {
    const std::string_view view(entry.data(), entry.size());
    return _entryEquals(view, key);
  }

  CommaSepComparator postCmp({ Limitor::Greater });
  CommaSepComparator commentCmp({ Limitor::Greater, Limitor::Less });
  CommaSepComparator entryCmp({ Limitor::Less, Limitor::Greater });
  CommaSepComparator userCmp({ Limitor::Less, Limitor::Less });
  CommaSepComparator wordsCmp({ Limitor::Less });
  CommaSepComparator indexCmp({ Limitor::Less, Limitor::Greater }); // List from newer posts

  leveldb::DB *postDB;
  leveldb::DB *commentDB;
  leveldb::DB *entryDB;
  leveldb::DB *userDB;
  leveldb::DB *wordsDB;
  leveldb::DB *indexDB;

  Post::Post(
      const std::string &uident,
      const std::string &url,
      const std::string &topic,
      const std::string &content,
      const std::vector<std::string> &tags,
      uint64_t post_time,
      uint64_t update_time) :
        uident(uident), url(url), topic(topic), content(content), tags(tags), post_time(post_time), update_time(update_time) {}

  Post::Post(const std::string_view &json) {
    static thread_local rj::Reader reader;
    SAX::PostSAXReader handler(*this);
    BoundedStringStream ss(json.data(), json.size());

    if(reader.Parse(ss, handler).IsError())
      throw StorageExcept::ParseError;

    // Not updated yet. For backward compatibility
    if(update_time == 0) update_time = post_time;
  }

  Post::Post(const std::string_view &json, const std::string &uident) :
        uident(uident), post_time(current_time()) {
    static thread_local rj::Reader reader;
    SAX::PostSAXReader handler(*this, true);
    BoundedStringStream ss(json.data(), json.size());

    if(reader.Parse(ss, handler).IsError())
      throw StorageExcept::ParseError;

    update_time = post_time;
  }

  void Post::write_json(rj::Writer<rj::StringBuffer> &w, bool border) const {
    if(border) w.StartObject();

    w.Key("uident");
    w.String(uident);
    w.Key("url");
    w.String(url);
    w.Key("topic");
    w.String(topic);
    w.Key("content");
    w.String(content);
    w.Key("post_time");
    w.Uint64(post_time);
    w.Key("update_time");
    w.Uint64(update_time);

    w.Key("tags");
    w.StartArray();
    for(auto &tag : tags) w.String(tag);
    w.EndArray();

    if(border) w.EndObject();
  }

  std::string Post::to_json(void) const {
    rj::StringBuffer buf;
    rj::Writer<rj::StringBuffer> w(buf);
    write_json(w);
    return buf.GetString();
  }

  Comment::Comment(
      const std::string &uident,
      const std::string &content,
      uint64_t comment_time) :
        uident(uident), content(content), comment_time(comment_time) { }

  Comment::Comment(const std::string_view &json) {
    rj::Document doc;
    if(doc.Parse(json.data(), json.size()).HasParseError()) throw StorageExcept::ParseError;

    uident = doc["uident"].GetString();
    content = doc["content"].GetString();
    comment_time = doc["comment_time"].GetUint64();
  }

  Comment::Comment(const std::string_view &json, const std::string &uident) :
        uident(uident), comment_time(current_time()) {
    rj::Document doc;
    if(doc.Parse(json.data(), json.size()).HasParseError()) throw StorageExcept::ParseError;

    content = doc["content"].GetString();
  }

  void Comment::write_json(rj::Writer<rj::StringBuffer> &w, bool border) const {
    if(border) w.StartObject();

    w.Key("uident");
    w.String(uident);
    w.Key("content");
    w.String(content);
    w.Key("comment_time");
    w.Uint64(comment_time);
    if(border) w.EndObject();
  }

  std::string Comment::to_json(void) const {
    rj::StringBuffer buf;
    rj::Writer<rj::StringBuffer> w(buf);
    write_json(w);
    return buf.GetString();
  }

  User::User(
      UserType type,
      const std::string &id,
      const std::string &name,
      const std::string &email,
      const std::string &avatar) :
        type(type), id(id), name(name), email(email), avatar(avatar) { }

  User::User(const std::string_view &json) {
    // TODO: use SAX instread
    rj::Document doc;
    if(doc.Parse(json.data(), json.size()).HasParseError()) throw StorageExcept::ParseError;

    // Type
    std::string typestr = doc["type"].GetString();
    if(typestr == "google") type = User::UserType::uGoogle;
    else type = User::UserType::uUnknown;

    id = doc["id"].GetString();
    name = doc["name"].GetString();
    email = doc["email"].GetString();
    avatar = doc["avatar"].GetString();
  }

  void User::write_json(rj::Writer<rj::StringBuffer> &w, bool border) const {
    if(border) w.StartObject();

    w.Key("type");
    if(type == User::UserType::uGoogle) w.String("google");
    else w.String("unknown");

    w.Key("id");
    w.String(id);
    w.Key("name");
    w.String(name);
    w.Key("email");
    w.String(email);
    w.Key("avatar");
    w.String(avatar);

    if(border) w.EndObject();
  }

  std::string User::to_json(void) const {
    rj::StringBuffer buf;
    rj::Writer<rj::StringBuffer> w(buf);
    write_json(w);
    return buf.GetString();
  }

  CommaSepComparator::CommaSepComparator(std::initializer_list<Limitor> lims) :
     lims(lims) { }

  int CommaSepComparator::Compare(const leveldb::Slice &a, const leveldb::Slice &b) const {
    const char *aptr = a.data(), *bptr = b.data();
    size_t al = a.size(), bl = b.size();

    auto lim = this->lims.cbegin();

    while(true) {
      if(!al) {
        if(!bl)
          return 0;
        if(*bptr == ',') return -1;
        return *lim == Limitor::Less ? -1 : 1;
      } else if(!bl) {
        if(*aptr == ',') return 1;
        return *lim == Limitor::Less ? 1 : -1;
      }

      if(*aptr != *bptr) {
        if(*aptr == ',')
          return *lim == Limitor::Less ? -1 : 1;
        if(*bptr == ',')
          return *lim == Limitor::Less ? 1 : -1;
        if((unsigned) *aptr < (unsigned) *bptr)
          return *lim == Limitor::Less ? -1 : 1;
        return *lim == Limitor::Less ? 1 : -1;
      } else if(*aptr == ',') ++lim;

      ++aptr;
      ++bptr;
      --al;
      --bl;
    }

#undef FASTFORWARD
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
    INIT_DB(comment);
    INIT_DB(entry);
    INIT_DB(user);
    INIT_DB(words);
    INIT_DB(index);

    return true;
  }

  bool setup_url_map(void) {
    std::unique_ptr<leveldb::Iterator> it(postDB->NewIterator(leveldb::ReadOptions()));

    for(it->SeekToFirst(); it->Valid(); it->Next()) {
      try {
        Post p(toStringView(it->value()));
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
    delete commentDB;
    delete entryDB;
    delete userDB;
    delete wordsDB;
    delete indexDB;
  }

  bool check_authors(void) {
    std::unique_ptr<leveldb::Iterator> it(postDB->NewIterator(leveldb::ReadOptions()));
    it->SeekToFirst();

    std::string defaultValue = "";
    std::string inputValue;
    std::string authorBuf;

    for(it->SeekToFirst() ;it->Valid(); it->Next()) {
      Post p = Post(toStringView(it->value()));
      leveldb::Status s = userDB->Get(leveldb::ReadOptions(), p.uident,&authorBuf);
      if(s.ok()) continue;
      else if(!s.IsNotFound()) return false;

      if(p.uident.length() > 0) defaultValue = p.uident;
      std::cout<<"Invalid user \""<<p.uident<<"\" for post: "<<p.topic<<std::endl;
      std::cout<<"Input a new user";
      if(defaultValue.length() > 0)
        std::cout<<" ["<<defaultValue<<"]: ";
      else std::cout<<": ";

      std::getline(std::cin, inputValue);
      if(std::cin.eof()) return false;
      else if(!inputValue.empty()) defaultValue = inputValue;
      
      p.uident = defaultValue;
      leveldb::Status ws = postDB->Put(leveldb::WriteOptions(), std::to_string(p.post_time), p.to_json());
      if(!ws.ok()) return false;
    }

    return it->status().ok();
  }

  /* Posts */

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
      if(s.IsNotFound()) throw StorageExcept::NotFound;
      else throw s;
    }
  }

  void update_post(const uint64_t &id, const Post &post) {
    if(!(post.post_time == id)) throw StorageExcept::IDMismatch;

    // TODO: use r-value reference
    Post np = post;
    np.update_time = current_time();
    leveldb::Status s = postDB->Put(leveldb::WriteOptions(), std::to_string(id), np.to_json());
    if(!s.ok()) throw s;
  }
  
  void delete_post(const uint64_t &id) {
    leveldb::Status s = postDB->Delete(leveldb::WriteOptions(), std::to_string(id));
    if(!s.ok()) {
      if(s.IsNotFound()) throw StorageExcept::NotFound;
      else throw s;
    }
  }

  std::vector<Post> list_posts(int offset, int count, bool &hasNext, uint64_t &total) {
    std::unique_ptr<leveldb::Iterator> it(postDB->NewIterator(leveldb::ReadOptions()));
    it->SeekToFirst();

    std::vector<Post> result;
    // TODO: cache total count
    if(count > 0) result.reserve(count);
    total = 0;

    while(it->Valid() && offset-- > 0) {
      ++total;
      it->Next();
    }

    for(int i = 0; (count == -1 || i < count) && it->Valid(); ++i) {
      result.emplace_back(toStringView(it->value()));
      ++total;
      it->Next();
    }

    hasNext = it->Valid();

    while(it->Valid()) {
      ++total;
      it->Next();
    }

    return result;
  }

  /* Comments */

  uint64_t add_comment(const uint64_t post_id, const Comment &comment) {
    const std::string id = std::to_string(post_id) + ',' + std::to_string(comment.comment_time);
    
    //Assume that we can't submit two post at the same millisecond 
    leveldb::Status s = postDB->Put(leveldb::WriteOptions(), id, comment.to_json());
    if(s.ok()) return comment.comment_time;
    else {
      throw s;
    }
  }

  std::vector<Comment> get_comments(uint64_t post_id, int offset, int count, bool &hasNext, uint64_t &total) {
    std::unique_ptr<leveldb::Iterator> it(entryDB->NewIterator(leveldb::ReadOptions()));
    const auto str_post_id = std::to_string(post_id);
    it->Seek(str_post_id);

    std::vector<Comment> result;
    if(count > 0) result.reserve(count);
    total = 0;

    while(it->Valid() && _entryEquals(it->key(), str_post_id) && offset-- > 0) {
      ++total;
      it->Next();
    }

    for(int i = 0; (count == -1 || i < count) && it->Valid() && _entryEquals(it->key(), str_post_id); ++i) {
      result.emplace_back(toStringView(it->value()));
      ++total;
      it->Next();
    }

    hasNext = it->Valid() && _entryEquals(it->key(), str_post_id);

    while(it->Valid() && _entryEquals(it->key(), str_post_id)) {
      ++total;
      it->Next();
    }

    return result;
  }

  void delete_comment(uint64_t post_id, uint64_t comment_id) {
    // TODO: implement
  }

  /* Entries */
  void _generate_remove_entries(const uint64_t &id, const std::vector<std::string> &list, leveldb::WriteBatch &batch) {
    for(auto &it : list)
      batch.Delete(it + "," + std::to_string(id));
  }

  void _generate_add_entries(const uint64_t &id, const std::vector<std::string> &list, leveldb::WriteBatch &batch) {
    for(auto &it : list)
      batch.Put(it + "," + std::to_string(id), std::to_string(id));
  }

  void remove_entries(const uint64_t &id, const std::vector<std::string> &list) {
    leveldb::WriteBatch batch;
    _generate_remove_entries(id, list, batch);
    entryDB->Write(leveldb::WriteOptions(), &batch);
  }

  void add_entries(const uint64_t &id, const std::vector<std::string> &list) {
    leveldb::WriteBatch batch;
    _generate_add_entries(id, list, batch);
    entryDB->Write(leveldb::WriteOptions(), &batch);
  }

  void add_remove_entries(const uint64_t &id, const std::vector<std::string> &add, const std::vector<std::string> &remove) {
    leveldb::WriteBatch batch;
    _generate_add_entries(id, add, batch);
    _generate_remove_entries(id, remove, batch);
    /* TODO: status */
    entryDB->Write(leveldb::WriteOptions(), &batch);
  }

  std::vector<uint64_t> list_posts_by_tag(const std::string &entry, int offset, int count, bool &hasNext, uint64_t &total) {
    std::unique_ptr<leveldb::Iterator> it(entryDB->NewIterator(leveldb::ReadOptions()));
    it->Seek(entry);

    std::vector<uint64_t> result;
    if(count > 0) result.reserve(count);
    total = 0;

    while(it->Valid() && _entryEquals(it->key(), entry) && offset-- > 0) {
      ++total;
      it->Next();
    }

    for(int i = 0; (count == -1 || i < count) && it->Valid() && _entryEquals(it->key(), entry); ++i) {
      const auto view = toStringView(it->value());
      uint64_t id;
      // TODO: check for errors
      std::from_chars(view.data(), view.data() + view.size(), id);
      result.push_back(id);
      ++total;
      it->Next();
    }

    hasNext = it->Valid() && _entryEquals(it->key(), entry);

    while(it->Valid() && _entryEquals(it->key(), entry)) {
      ++total;
      it->Next();
    }

    return result;
  }

  /* Users */
  bool update_user(const User &user) {
    leveldb::Status s = userDB->Put(leveldb::WriteOptions(), user.getKey(), user.to_json());
    return s.ok();
  }

  std::string get_user_str(const std::string &uident) {
    std::string v;
    leveldb::Status s = userDB->Get(leveldb::ReadOptions(), uident, &v);
    if(s.ok()) return v;
    else {
      if(s.IsNotFound()) throw StorageExcept::NotFound;
      else throw s;
    }
  }

  User get_user(const std::string &uident) {
    return User(get_user_str(uident));
  }

  /* Index */
  void set_indexes(uint64_t post, const std::unordered_map<std::string, std::vector<std::pair<uint32_t, bool>>> &indexes) {
    leveldb::WriteBatch indexBatch;

    std::string words;
    leveldb::Status s = wordsDB->Get(leveldb::ReadOptions(), std::to_string(post), &words);
    if(!s.ok()) {
      if(!s.IsNotFound()) throw s;
    } else {
      std::stringstream ws(words);
      std::string w;
      while(ws>>w)
        indexBatch.Delete(w + ',' + std::to_string(post));
    }

    std::stringstream curWords;
    for(auto &it : indexes) {
      std::stringstream indexes;
      for(auto &occur : it.second)
        indexes<<occur.first<<' '<<(occur.second ? 't' : 'b')<<'\n';
      indexBatch.Put(it.first + ',' + std::to_string(post), indexes.str());
      curWords<<it.first<<'\n';
    }

    leveldb::Status is = indexDB->Write(leveldb::WriteOptions(), &indexBatch);
    if(!is.ok()) throw is;
    wordsDB->Put(leveldb::WriteOptions(), std::to_string(post), curWords.str());
  }

  void clear_indexes(uint64_t post) {
    leveldb::WriteBatch batch;

    std::string words;
    leveldb::Status s = wordsDB->Get(leveldb::ReadOptions(), std::to_string(post), &words);
    if(!s.ok()) {
      if(s.IsNotFound()) throw StorageExcept::NotFound;
      else throw s;
    }

    std::stringstream ws(words);
    std::string w;
    while(ws>>w)
      batch.Delete(w + ',' + std::to_string(post));

    indexDB->Write(leveldb::WriteOptions(), &batch);
    wordsDB->Delete(leveldb::WriteOptions(), std::to_string(post));
  }

  std::unordered_map<uint64_t, std::vector<std::pair<uint32_t, bool>>> query_indexes(const std::string &str) {
    std::unique_ptr<leveldb::Iterator> it(indexDB->NewIterator(leveldb::ReadOptions()));
    it->Seek(str);

    std::unordered_map<uint64_t, std::vector<std::pair<uint32_t, bool>>> res;
    for(; it->Valid(); it->Next()) {
      auto keySegs = split(it->key().ToString(), ',');
      auto keyIter = keySegs.begin();
      if(*keyIter != str) break;

      std::stringstream ss(it->value().ToString());
      uint32_t v;
      char f;
      std::vector<std::pair<uint32_t, bool>> l;
      while(ss>>v>>f)
        l.emplace_back(v, f == 't');

      ++keyIter;
      res.emplace(std::stoull(*keyIter), std::move(l));
    }

    return res;
  }
}
