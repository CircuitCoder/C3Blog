#include <rapidjson/reader.h>
#include <cstring>

#include "storage.h"

namespace rj = rapidjson;

namespace C3 {
  namespace SAX {
    template <typename D>
    struct KeyTrieNode {
      KeyTrieNode* children[128];
      D data;
    };

    template <typename D>
    bool setTrie(KeyTrieNode<D> *root, const char *path, int length, D data) {
      if(length < 0) length = std::strlen(path);
      while(length > 0) {
        if(*path < 0) return false;

        if(!root->children[(size_t) *path]) root->children[(size_t) *path] = new KeyTrieNode<D>();
        root = root->children[(size_t) *path];
        ++path;
        --length;
      }
      root->data = data;
      return true;
    }

    template <typename D>
    D getTrie(KeyTrieNode<D> *root, const char *path, int length, D failed) {
      if(length < 0) length = std::strlen(path);
      while(length > 0) {
        if(*path < 0) return failed;
        root = root->children[(size_t) *path];
        if(!root) return failed;
        ++path;
        --length;
      }
      return root->data;
    }

    enum class PostKey {
      Tags, UIdent, URL, Topic, Content, PostTime, UpdateTime, _IDLE,
    };

    void setup();

    class PostSAXReader : public rj::BaseReaderHandler<rj::UTF8<>, PostSAXReader> {
    private:
      // TODO: validation
      Post &_target;
      PostKey _status;
      bool _ready = false;
      bool _newPost;

    public:
      PostSAXReader(Post &target, bool newPost = false);
      bool Key(const char *key, size_t length, bool copy);
      bool Uint64(uint64_t i);
      bool Int(int i);
      bool Uint(int i);
      bool Int64(int i);
      bool String(const char *str, size_t length, bool copy);

      bool StartArray();
      bool EndArray(size_t s);
    };
  }
}
