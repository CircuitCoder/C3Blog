#include <cstdint>
#include <sstream>
#include <chrono>

extern "C" {
#include <mkdio.h>
}

#include "util.h"

namespace C3 {
  Randomizer::Randomizer(uint64_t a, uint64_t b) : mt(rd()), dist(a,b) { }

  uint64_t Randomizer::next(void) {
    return dist(mt);
  }

  uint64_t current_time(void) {
    return std::chrono::duration_cast<std::chrono::duration<uint64_t, std::milli>>
      (std::chrono::system_clock::now().time_since_epoch()).count();
  }

  std::vector<std::string> split(const std::string &s, char delim) {
    //TODO: use list
    std::vector<std::string> elems;
    std::stringstream ss(s);
    std::string item;
    while(std::getline(ss, item, delim)) elems.push_back(item);
    return elems;
  }

  std::string charset("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890");
  Randomizer char_rand(0, 26*2 + 10-1);

  std::string random_chars(int length) {
    std::string result;
    while(length --)
      result.push_back(charset[char_rand.next()]);
    return result;
  }

  std::string markdown(std::string src) {
    auto flags = MKD_AUTOLINK;
    auto doc = mkd_string(src.c_str(), src.length(), flags);

    if(mkd_compile(doc, flags)) {
      char * buf;
      int szdoc = mkd_document(doc, &buf);
      mkd_cleanup(doc);
      return std::string(buf);
    } else {
      throw "Conversion Failed";
    }
  }

  namespace URLEncoding {

    // From http://www.geekhideout.com/urlcode.shtml

    /* Converts a hex character to its integer value */
    char from_hex(char ch) {
      return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
    }

    /* Converts an integer value to its hex character*/
    char to_hex(char code) {
      static char hex[] = "0123456789abcdef";
      return hex[code & 15];
    }

    /* Returns a url-encoded version of str */
    std::string url_encode(std::string str) {
      auto pstr = str.begin();
      std::stringstream buf;

      while (pstr != str.end()) {
        if (isalnum(*pstr) || *pstr == '-' || *pstr == '_' || *pstr == '.' || *pstr == '~') 
          buf<<*pstr;
        else if (*pstr == ' ') 
          buf<<'+';
        else 
          buf<<'%'<<to_hex(*pstr >> 4)<<to_hex(*pstr & 15);

        ++pstr;
      }

      return buf.str();
    }

    /* Returns a url-decoded version of str */
    std::string url_decode(std::string str) {
      auto pstr = str.begin();
      std::stringstream buf;

      while (pstr != str.end()) {
        if (*pstr == '%') {
          if (pstr + 1 != str.end() && pstr + 2 != str.end()) {
            buf<<(char) (from_hex(*(pstr + 1)) << 4 | from_hex(*(pstr + 2)));
            pstr += 2;
          }
        } else if (*pstr == '+') { 
          buf<<' ';
        } else {
          buf<<*pstr;
        }

        ++pstr;
      }

      return buf.str();
    }
  }
}
