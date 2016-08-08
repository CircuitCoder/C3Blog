#include <tinyxml2.h>
#include <chrono>
#include <sstream>
#include <ctime>

#include "feed.h"

#include "storage.h"

namespace C3 {
  namespace Feed {
    std::string feed_str = "";
    uint16_t feed_length;
    std::string title;
    std::string url;

    std::string generate_rfc3999(uint64_t ms_since_epoch) {
      std::chrono::milliseconds duration(ms_since_epoch);
      std::chrono::time_point<std::chrono::system_clock> point(duration);
      time_t tt = std::chrono::system_clock::to_time_t(point);

      std::tm *cal = gmtime(&tt);

      std::stringstream ss;
      ss<<cal->tm_year + 1900<<'-'<<cal->tm_mon + 1<<'-'<<cal->tm_mday<<'T'<<cal->tm_hour<<':'<<cal->tm_min<<':'<<cal->tm_sec<<'Z';
      return ss.str();
    }

    void setup(const Config &c) {
      feed_length = c.app_feedLength;
      title = c.app_title;
      url = c.app_url;
      update();
    }

    void update(void) {
      bool dummy;
      auto posts = list_posts(0, feed_length, dummy);

      tinyxml2::XMLDocument doc;

      auto dec = doc.NewDeclaration();
      auto root = doc.NewElement("feed");
      root->SetAttribute("xmlns", "http://www.w3.org/2005/Atom");

      auto id_e = doc.NewElement("id");
      id_e->SetText(url.c_str());
      root->InsertEndChild(id_e);

      auto title_e = doc.NewElement("title");
      title_e->SetText(title.c_str());
      root->InsertEndChild(title_e);

      auto link_e = doc.NewElement("link");
      link_e->SetAttribute("href", url.c_str());
      root->InsertEndChild(link_e);

      auto dur = std::chrono::system_clock::now().time_since_epoch();
      auto durStr = generate_rfc3999(std::chrono::duration_cast<std::chrono::milliseconds>(dur).count());
      auto updated_e = doc.NewElement("updated");
      updated_e->SetText(durStr.c_str());
      root->InsertEndChild(updated_e);

      for(auto i = posts.begin(); i != posts.end(); ++i) {
        auto entry_e = doc.NewElement("entry");
        
        auto entry_id_e = doc.NewElement("id");
        entry_id_e->SetText((url + "[" + std::to_string(i->post_time) + "]").c_str());
        entry_e->InsertEndChild(entry_id_e);

        auto entry_title_e = doc.NewElement("title");
        entry_title_e->SetText(i->topic.c_str());
        entry_e->InsertEndChild(entry_title_e);

        auto entry_content_e = doc.NewElement("content");
        entry_content_e->SetText(markdown(i->content).c_str());
        entry_content_e->SetAttribute("type", "html");
        entry_e->InsertEndChild(entry_content_e);

        auto entry_updated_e = doc.NewElement("updated");
        entry_updated_e->SetText(generate_rfc3999(i->update_time).c_str());
        entry_e->InsertEndChild(entry_updated_e);

        auto entry_link_e = doc.NewElement("link");
        entry_link_e->SetAttribute("href", (url + "/" + i->url).c_str());
        entry_link_e->SetAttribute("rel", "alternative");
        entry_e->InsertEndChild(entry_link_e);

        auto entry_link_fb = doc.NewElement("link");
        entry_link_fb->SetText((url + "/" + i->url).c_str());
        entry_e->InsertEndChild(entry_link_fb);

        root->InsertEndChild(entry_e);
      }

      doc.InsertEndChild(dec);
      doc.InsertEndChild(root);

      tinyxml2::XMLPrinter printer(NULL, true, 0);
      doc.Print(&printer);
      feed_str = printer.CStr();
    }

    std::string fetch(void) {
      return feed_str;
    }
  }
}
