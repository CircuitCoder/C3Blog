#include <string>
#include <crow.h>
#include <json/json.h>

#include "indexer.h"
#include "util.h"

namespace C3 {
  extern Json::StreamWriterBuilder wbuilder;

  void handle_search(const crow::request &req, crow::response &res, std::string str);

  void handle_search(const crow::request &req, crow::response &res, std::string str) {
    auto records = search_index(URLEncoding::url_decode(str));
    Json::Value root(Json::arrayValue);

    for(auto rec = records.begin(); rec != records.end(); ++rec) {
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
      root.append(recJson);
    }

    res.end(Json::writeString(wbuilder, root));
  }
}
