#include <string>
#include <crow.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "indexer.h"
#include "util.h"
#include "handlers/search.h"

namespace rj = rapidjson;

namespace C3 {
  uint32_t search_preview, search_length;

  void setup_search_handler(const Config &c) {
    search_preview = c.search_preview;
    search_length = c.search_length;
  }

  void handle_search(const crow::request &req, crow::response &res, std::string str) {
    handle_search_page(req, res, str, 1);
  }

  void handle_search_page(const crow::request &req, crow::response &res, std::string str, uint64_t page) {
    auto records = Index::search(URLEncoding::url_decode(str));
    rj::StringBuffer result;
    rj::Writer<rj::StringBuffer> writer(result);
    writer.StartObject(); // Root
    writer.Key("results");
    writer.StartArray(); // Results

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

      writer.StartObject(); // Record
      writer.Key("post_id");
      writer.Uint64(rec->first);

      writer.Key("hits");
      writer.StartArray(); // Hits

      uint32_t offsetPtr = 0;
      uint32_t offsetUTF8 = 0;

      for(auto &hit : rec->second) {
        uint32_t offsetAscii = std::get<0>(hit);

        while(offsetPtr < offsetAscii)
          if((p.content[offsetPtr++] & 0xC0) != 0x80) ++offsetUTF8;

        uint32_t lengthAscii = std::get<1>(hit);
        uint32_t lengthPtr = 0;
        uint32_t lengthUTF8 = 0;

        while(lengthPtr < lengthAscii)
          if((p.content[offsetPtr + (lengthPtr++)] & 0xC0) != 0x80) ++lengthUTF8;

        writer.StartObject(); // Hit
        writer.Key("offset");
        writer.Uint(offsetUTF8);
        writer.Key("length");
        writer.Uint(lengthUTF8);
        writer.Key("title");
        writer.Bool(std::get<2>(hit));
        writer.EndObject(); // Hit

        if(!std::get<2>(hit)){
          inbody = true;
          while(std::get<0>(hit) > lineEnd) {
            ++lineCount;
            ++lineEnd;
            while(lineEnd < p.content.length() && p.content[lineEnd] != '\n')
              ++lineEnd;
            lineEnds.push_back(lineEnd);
            lineHits.push_back(0);
          }
        }

        ++lineHits[lineCount];
      }

      writer.EndArray(); // Hits

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

        writer.Key("preview");
        writer.String(p.content.substr(startIndex, endIndex - startIndex));
      } else { // Only in title
        uint32_t currentLine = 0;
        uint32_t ptr = 0;
        while(ptr < p.content.size() && currentLine < search_preview) {
          while(ptr < p.content.size() && p.content[ptr] != '\n') ++ptr;
          ++currentLine;
          ++ptr;
        }

        writer.Key("preview");
        writer.String(p.content.substr(0, ptr - 1));
      }

      writer.Key("tags");
      writer.StartArray(); // Tags
      for(auto &tag : p.tags) writer.String(tag);
      writer.EndArray(); // Tags
      writer.Key("url");
      writer.String(p.url);
      writer.Key("updated");
      writer.Uint64(p.update_time);
      writer.EndObject(); // Record

      ++i;
    }

    writer.EndArray(); // Results
    writer.Key("pages");
    writer.Uint64((records.size() + search_length - 1) / search_length);

    res.end(result.GetString());
  }
}
