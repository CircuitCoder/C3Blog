#include <string>
#include <crow.h>
#include <json/json.h>

#include "indexer.h"
#include "util.h"

namespace C3 {
  extern Json::StreamWriterBuilder wbuilder;

  uint32_t search_preview, search_length;

  void setup_search_handler(const Config &c) {
    search_preview = c.search_preview;
    search_length = c.search_length;
  }

  void handle_search(const crow::request &req, crow::response &res, std::string str);
  void handle_search_page(const crow::request &req, crow::response &res, std::string str, uint64_t page);

  void handle_search(const crow::request &req, crow::response &res, std::string str) {
    handle_search_page(req, res, str, 1);
  }

  void handle_search_page(const crow::request &req, crow::response &res, std::string str, uint64_t page) {
    auto records = search_index(URLEncoding::url_decode(str));
    Json::Value recordsJson(Json::arrayValue);

    auto rec = records.begin();
    uint32_t skipped = search_length * (page - 1);
    while(rec != records.end() && skipped) {
      --skipped;
      ++rec;
    }

    int i = 0;

    for(; rec != records.end() && i < search_length; ++rec) {
      Json::Value recJson;
      Json::Value hitsJson(Json::arrayValue);
      recJson["post_id"] = rec->first;
      for(auto hit = rec->second.begin(); hit != rec->second.end(); ++hit) {
        Json::Value hitJson;
        hitJson["offset"] = std::get<0>(*hit);
        hitJson["length"] = std::get<1>(*hit);
        hitJson["title"] = std::get<2>(*hit);

        hitsJson.append(hitJson);
      }

      recJson["hits"] = hitsJson;
      recordsJson.append(recJson);

      ++i;
    }

    Json::Value root;

    root["pages"] = ((uint64_t) records.size() + search_length - 1) / search_length;
    root["results"] = recordsJson;

    res.end(Json::writeString(wbuilder, root));
  }
}
