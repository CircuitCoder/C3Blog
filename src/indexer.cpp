#include "indexer.h"

#include <cppjieba/Jieba.hpp>
#include <vector>
#include <list>
#include <unordered_map>
#include <algorithm>
#include <mutex>

#include "util.h"

namespace C3 {
  namespace Index {
    cppjieba::Jieba *jieba;

    typedef std::vector<std::pair<uint64_t, std::list<std::tuple<uint32_t, uint32_t, bool>>>> search_result;
    uint32_t search_cache_size;
    // TODO: switch to vector
    std::list<std::string> search_cache_list;
    std::unordered_map<std::string, std::pair<search_result, std::list<std::string>::iterator>> search_cache_store;
    std::mutex search_cache_mutex;

    bool _index_cmp_pair(const std::pair<uint32_t, bool> &a, const std::tuple<uint32_t, uint32_t, bool> &b) {
      if(a.second) {
        if(std::get<2>(b)) return a.first < std::get<0>(b);
        else return true;
      } if(std::get<2>(b)) return false;

      return a.first < std::get<0>(b);
    }

    bool _index_cmp(const std::tuple<uint32_t, uint32_t, bool> &a, const std::tuple<uint32_t, uint32_t, bool> &b) {
      if(std::get<2>(a)) {
        if(std::get<2>(b)) return std::get<0>(a) < std::get<0>(b);
        else return true;
      } if(std::get<2>(b)) return false;

      return std::get<0>(a) < std::get<0>(b);
    }

    void setup(const Config &c) {
      search_cache_size = c.search_cache;
      jieba = new cppjieba::Jieba(
          c.search_dict + "/jieba.dict.utf8",
          c.search_dict + "/hmm_model.utf8",
          c.search_dict + "/user.dict.utf8",
          c.search_dict + "/idf.utf8",
          c.search_dict + "/stop_words.utf8");
    }

    void reindex(const Post& p) {
      auto indexes = generate(p.topic, p.content);
      set_indexes(p.post_time, indexes);
      invalidate();
    }

    void reindex_all(void) {
      bool dummy_hasNext;
      uint64_t dummy_total;
      auto posts = list_posts(0, -1, dummy_hasNext, dummy_total);
      for(auto &p : posts)
        reindex(p);
    }

    void invalidate() {
      std::unique_lock<std::mutex> lock(search_cache_mutex);

      // TODO: optimize: only invalidate results containing the post
      search_cache_list.clear();
      search_cache_store.clear();
    }

    std::unordered_map<std::string, std::vector<std::pair<uint32_t, bool>>>
      generate(const std::string &title, const std::string &body) {
        std::vector<cppjieba::Word> titleWords, bodyWords;
        jieba->CutForSearch(title, titleWords, true);
        jieba->CutForSearch(body, bodyWords, true);
        std::unordered_map<std::string, std::vector<std::pair<uint32_t, bool>>> map;

        for(auto &seg : titleWords)
          map[seg.word].emplace_back(seg.offset, true);
        for(auto &seg : bodyWords)
          map[seg.word].emplace_back(seg.offset, false);

        return map;
      }

    search_result search(std::string target) {
      std::unique_lock<std::mutex> lock(search_cache_mutex);
      if(search_cache_store.count(target) > 0) {
        auto &cached = search_cache_store[target];
        search_cache_list.splice(search_cache_list.end(), search_cache_list, cached.second);
        return cached.first;
      }
      lock.unlock();

      auto segs = split(target, ' ');

      std::unordered_map<uint64_t, std::list<std::tuple<uint32_t, uint32_t, bool>>> tot;

      for(auto &seg : segs) {
        std::unordered_map<uint64_t, std::list<std::tuple<uint32_t, uint32_t, bool>>> curRes;

        std::vector<std::string> words;
        jieba->Cut(seg, words, true);

        bool isBegin = true;
        
        for(auto &word : words) {
          auto store = query_indexes(word);

          if(isBegin) {
            isBegin = false;
            for(auto &rec : store)
              for(auto &occur : rec.second)
                curRes[rec.first]
                  .emplace_back(occur.first, word.length(), occur.second);
          } else for(auto res = curRes.begin(); res != curRes.end(); ++res) {
            while(res != curRes.end() && store.count(res->first) == 0)
              res = curRes.erase(res);

            if(res == curRes.end()) break;

            const std::vector<std::pair<uint32_t, bool>> &storeOccurs = store[res->first];
            auto storeOccurIter = storeOccurs.begin();

            for(auto occur = res->second.begin(); occur != res->second.end(); ++occur) {
              while(_index_cmp_pair(*storeOccurIter, *occur)) {
                res->second.emplace(occur, storeOccurIter->first, word.length(), storeOccurIter->second);
                ++storeOccurIter;
                if(storeOccurIter == storeOccurs.end()) break;
              }
              if(storeOccurIter == storeOccurs.end()) break;
            }

            for(;storeOccurIter != storeOccurs.end(); ++storeOccurIter)
              res->second.emplace_back(storeOccurIter->first, word.length(), storeOccurIter->second);
          }
        }

        for(auto &res : curRes) {
          if(tot.count(res.first) == 0) tot.emplace(res.first, std::move(res.second));
          else tot[res.first].merge(res.second, _index_cmp);
        }
      }

      std::vector<std::pair<uint64_t, uint64_t>> scores(tot.size());
      int i = 0;
      for(auto &row : tot)
        scores[i++] = std::make_pair(row.second.size(), row.first);

      sort(scores.begin(), scores.end(), std::greater<std::pair<uint64_t, uint64_t>>());

      search_result records;
      for(auto &rec : scores)
        records.emplace_back(rec.second, std::move(tot[rec.second]));

      lock.lock();
      if(search_cache_store.count(target) == 0) {
        while(search_cache_list.size() >= search_cache_size) {
          // Pop the first element
          search_cache_store.erase(search_cache_list.front());
          search_cache_list.pop_front();
        }

        search_cache_list.emplace_back(target);
        auto iter = std::prev(search_cache_list.end());
        search_cache_store.emplace(target, std::make_pair(records, iter));
      }
      lock.unlock();

      return records;
    }
  }
}
