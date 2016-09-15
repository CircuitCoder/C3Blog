#include "indexer.h"

#include <cppjieba/Jieba.hpp>
#include <vector>
#include <algorithm>

#include "util.h"

namespace C3 {
  cppjieba::Jieba *jieba;

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

  void setup_indexer(const Config &c) {
    jieba = new cppjieba::Jieba(
        c.search_dict + "/jieba.dict.utf8",
        c.search_dict + "/hmm_model.utf8",
        c.search_dict + "/user.dict.utf8",
        c.search_dict + "/idf.utf8",
        c.search_dict + "/stop_words.utf8");
  }

  void reindex(const Post& p) {
    auto indexes = generate_indexes(p.topic, p.content);
    set_indexes(p.post_time, indexes);
    invalidate_index_cache(p.post_time);
  }

  void reindex_all(void) {
    bool dummy;
    auto posts = list_posts(0, -1, dummy);
    for(auto p = posts.begin(); p != posts.end(); ++p)
      reindex(*p);
  }

  void invalidate_index_cache(const uint64_t postid) {
    //TODO: implement cache
  }

  std::unordered_map<std::string, std::list<std::pair<uint32_t, bool>>>
    generate_indexes(const std::string &title, const std::string &body) {
      std::vector<cppjieba::Word> titleWords, bodyWords;
      jieba->CutForSearch(title, titleWords, true);
      jieba->CutForSearch(body, bodyWords, true);
      std::unordered_map<std::string, std::list<std::pair<uint32_t, bool>>> map;

      for(auto it = titleWords.begin(); it != titleWords.end(); ++it)
        map[it->word].emplace_back(it->offset, true);
      for(auto it = bodyWords.begin(); it != bodyWords.end(); ++it)
        map[it->word].emplace_back(it->offset, true);

      return map;
    }

  std::list<std::pair<uint64_t, std::list<std::tuple<uint32_t, uint32_t, bool>>>>
    search_index(std::string target) {
      auto segs = split(target, ' ');

      std::unordered_map<uint64_t, std::list<std::tuple<uint32_t, uint32_t, bool>>> tot;

      for(auto seg = segs.begin(); seg != segs.end(); ++seg) {
        std::unordered_map<uint64_t, std::list<std::tuple<uint32_t, uint32_t, bool>>> curRes;

        std::vector<std::string> words;
        jieba->Cut(*seg, words, true);
        
        for(auto word = words.begin(); word != words.end(); ++word) {
          auto store = query_indexes(*word);

          if(word == words.begin()) {
            for(auto rec = store.begin(); rec != store.end(); ++rec)
              for(auto occur = rec->second.begin(); occur != rec->second.end(); ++occur)
                curRes[rec->first].emplace_back(occur->first, word->length(), occur->second);
          } else for(auto res = curRes.begin(); res != curRes.end(); ++res) {
            while(res != curRes.end() && store.count(res->first) == 0)
              res = curRes.erase(res);

            if(res == curRes.end()) break;

            const std::list<std::pair<uint32_t, bool>> &storeOccurs = store[res->first];
            auto storeOccurIter = storeOccurs.begin();

            for(auto occur = res->second.begin(); occur != res->second.end(); ++occur) {
              while(_index_cmp_pair(*storeOccurIter, *occur)) {
                res->second.emplace(occur, storeOccurIter->first, word->length(), storeOccurIter->second);
                ++storeOccurIter;
                if(storeOccurIter == storeOccurs.end()) break;
              }
              if(storeOccurIter == storeOccurs.end()) break;
            }

            for(;storeOccurIter != storeOccurs.end(); ++storeOccurIter)
              res->second.emplace_back(storeOccurIter->first, word->length(), storeOccurIter->second);
          }
        }

        for(auto res = curRes.begin(); res != curRes.end(); ++res) {
          if(tot.count(res->first) == 0) tot.emplace(res->first, std::move(res->second));
          else tot[res->first].merge(res->second, _index_cmp);
        }
      }

      std::vector<std::pair<uint64_t, uint64_t>> scores(tot.size());
      int i = 0;
      for(auto row = tot.begin(); row != tot.end(); ++row)
        scores[i++] = std::make_pair(row->second.size(), row->first);

      sort(scores.begin(), scores.end());

      std::list<std::pair<uint64_t, std::list<std::tuple<uint32_t, uint32_t, bool>>>> records;
      for(auto rec = scores.begin(); rec != scores.end(); ++rec)
        records.emplace_back(rec->second, std::move(tot[rec->second]));

      return records;
    }
}
