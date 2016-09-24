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
    auto records = Index::search(URLEncoding::url_decode(str));
    Json::Value recordsJson(Json::arrayValue);

    auto rec = records.begin();
    uint32_t skipped = search_length * (page - 1);
    while(rec != records.end() && skipped) {
      --skipped;
      ++rec;
    }

    uint32_t i = 0;

    for(; rec != records.end() && i < search_length; ++rec) {

      Post p = get_post(rec->first);

      uint32_t lineEnd = 0, lineCount = 0;
      while(lineEnd < p.content.length() && p.content[lineEnd] != '\n') ++lineEnd;

      std::vector<uint32_t> lineHits(1);
      std::vector<uint32_t> lineEnds(1);
      lineEnds[0] = lineEnd;
      lineHits[0] = 0;
      bool inbody = false;

      Json::Value recJson;
      Json::Value hitsJson(Json::arrayValue);
      recJson["post_id"] = Json::Value::UInt64(rec->first);

      uint32_t offsetPtr = 0;
      uint32_t offsetUTF8 = 0;

      for(auto hit = rec->second.begin(); hit != rec->second.end(); ++hit) {
        Json::Value hitJson;
        uint32_t offsetAscii = std::get<0>(*hit);

        while(offsetPtr < offsetAscii)
          if((p.content[offsetPtr++] & 0xC0) != 0x80) ++offsetUTF8;

        uint32_t lengthAscii = std::get<1>(*hit);
        uint32_t lengthPtr = 0;
        uint32_t lengthUTF8 = 0;

        while(lengthPtr < lengthAscii)
          if((p.content[offsetPtr + (lengthPtr++)] & 0xC0) != 0x80) ++lengthUTF8;

        hitJson["offset"] = offsetUTF8;
        hitJson["length"] = lengthUTF8;
        hitJson["title"] = std::get<2>(*hit);

        if(!std::get<2>(*hit)){
          inbody = true;
          while(std::get<0>(*hit) > lineEnd) {
            ++lineCount;
            ++lineEnd;
            while(lineEnd < p.content.length() && p.content[lineEnd] != '\n')
              ++lineEnd;
            lineEnds.push_back(lineEnd);
            lineHits.push_back(0);
          }
        }

        ++lineHits[lineCount];

        hitsJson.append(hitJson);
      }

      ++lineCount;

      if(inbody) {
        uint32_t maxStart = 0;
        if(search_preview < lineCount) {
          uint32_t maxSum;
          uint32_t sum = 0;

          for(uint32_t i = 0; i < search_preview ; ++i) {
            sum += lineHits[i];
          }

          maxSum = sum;

          for(uint32_t start = 1; start + search_preview <= lineCount; ++start) {
            sum -= lineEnds[start - 1];
            sum += lineEnds[start - 1 + search_preview];
            if(sum > maxSum) {
              maxSum = sum;
              maxStart = start;
            }
          }
        }

        uint32_t maxEnd = maxStart + search_preview - 1;
        if(maxEnd >= lineCount) maxEnd = lineCount - 1;
        while(maxEnd > maxStart && lineHits[maxEnd] == 0) --maxEnd;
        while(maxStart < maxEnd && lineHits[maxStart] == 0) ++maxStart;

        const uint32_t startIndex = maxStart == 0 ? 0 : lineEnds[maxStart-1] +1;
        const uint32_t endIndex = lineEnds[maxEnd];

        recJson["preview"] = p.content.substr(startIndex, endIndex - startIndex);
      } else { // Only in title
        uint32_t currentLine = 0;
        uint32_t ptr = 0;
        while(ptr < p.content.size() && currentLine < search_preview) {
          while(ptr < p.content.size() && p.content[ptr] != '\n') ++ptr;
          ++currentLine;
          ++ptr;
        }

        recJson["preview"] = p.content.substr(0, ptr - 1);
      }

      recJson["hits"] = hitsJson;
      Json::Value tags(Json::arrayValue);
      for(auto tag : p.tags) tags.append(tag);
      recJson["tags"] = tags;
      recJson["url"] = p.url;
      recJson["updated"] = Json::UInt64(p.update_time);
      recordsJson.append(recJson);

      ++i;
    }

    Json::Value root;

    root["pages"] = Json::Value::UInt64((records.size() + search_length - 1) / search_length);
    root["results"] = recordsJson;

    res.end(Json::writeString(wbuilder, root));
  }
}
