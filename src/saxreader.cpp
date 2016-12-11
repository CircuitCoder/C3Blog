#include <rapidjson/reader.h>
#include <string>

#include "saxreader.h"

namespace C3 {
  namespace SAX {
    KeyTrieNode<PostKey> postRouter;

    void setup() {
      setTrie(&postRouter, "tags", -1, PostKey::Tags);
      setTrie(&postRouter, "uident", -1, PostKey::UIdent);
      setTrie(&postRouter, "url", -1, PostKey::URL);
      setTrie(&postRouter, "topic", -1, PostKey::Topic);
      setTrie(&postRouter, "content", -1, PostKey::Content);
      setTrie(&postRouter, "post_time", -1, PostKey::PostTime);
      setTrie(&postRouter, "update_time", -1, PostKey::UpdateTime);
    }

    PostSAXReader::PostSAXReader(Post &target, bool newPost) : _target(target), _status(PostKey::_IDLE), _newPost(newPost) {}
    bool PostSAXReader::Key(const char *key, size_t length, bool copy) {
      _status = getTrie(&postRouter, key, length, PostKey::_IDLE);
      if(_status == PostKey::Tags) _ready = false;
      else _ready = true;
      return true;
    }

    bool PostSAXReader::Uint64(uint64_t i) {
      if(!_ready) return false;
      if(_newPost) return false; // No timestamps are allowed when creating new posts

      _ready = false;
      if(_status == PostKey::PostTime) _target.post_time = i;
      else if(_status == PostKey::UpdateTime) _target.update_time = i;
      else return false;
      return true;
    }

    bool PostSAXReader::Int(int i) { return Uint64(i); }
    bool PostSAXReader::Uint(int i) { return Uint64(i); }
    bool PostSAXReader::Int64(int i) { return Uint64(i); }

    bool PostSAXReader::String(const char *str, size_t length, bool copy) {
      if(!_ready) return false;
      if(_status == PostKey::Tags) _target.tags.emplace_back(str, length);
      else {
        _ready = false;
        if(_status == PostKey::UIdent && !_newPost) _target.uident = std::string(str, length);
        else if(_status == PostKey::URL) _target.url = std::string(str, length);
        else if(_status == PostKey::Topic) _target.topic = std::string(str, length);
        else if(_status == PostKey::Content) _target.content = std::string(str, length);
        else return false;
      }
      return true;
    }

    bool PostSAXReader::StartArray() {
      if(_status != PostKey::Tags) return false;
      _ready = true;
      return true;
    }

    bool PostSAXReader::EndArray(size_t s) {
      if(_status != PostKey::Tags) return false;
      _ready = false;
      _status = PostKey::_IDLE;
      return true;
    }
  }
}
