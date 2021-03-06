#include <tinyxml2.h>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <ctime>

#include "feed.h"

#include "storage.h"
#include "util.h"

namespace C3 {
  namespace Feed {
    std::string atom_str;
    bool atom_valid = false;

    std::string sitemap_str;
    bool sitemap_valid = false;

    uint16_t feed_length;
    std::string title;
    std::string url;

    std::string generate_rfc3999(uint64_t ms_since_epoch) {
      std::chrono::milliseconds duration(ms_since_epoch);
      std::chrono::time_point<std::chrono::system_clock> point(duration);
      time_t tt = std::chrono::system_clock::to_time_t(point);

      std::tm *cal = gmtime(&tt);

      std::stringstream ss;
      ss.fill('0');
      ss
        << cal->tm_year + 1900 << '-' << std::setw(2) << cal->tm_mon + 1 << '-' << std::setw(2) << cal->tm_mday
        << 'T' << std::setw(2) << cal->tm_hour << ':' << std::setw(2) << cal->tm_min << ':' << std::setw(2) << cal->tm_sec << 'Z';
      return ss.str();
    }

    void setup(const Config &c) {
      feed_length = c.app_feedLength;
      title = c.app_title;
      url = c.app_url;

      if(url.c_str()[url.length() -1] != '/') url += '/';
    }

    void invalidate(void) {
      atom_valid = false;
      sitemap_valid = false;
    }

    void updateAtom(void) {
      bool dummy_hasNext;
      uint64_t dummy_total;
      auto posts = list_posts(0, feed_length, dummy_hasNext, dummy_total);

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

      auto self_e = doc.NewElement("link");
      self_e->SetAttribute("href", "/feed");
      self_e->SetAttribute("rel", "self");
      root->InsertEndChild(self_e);

      auto dur = std::chrono::system_clock::now().time_since_epoch();
      auto durStr = generate_rfc3999(std::chrono::duration_cast<std::chrono::milliseconds>(dur).count());
      auto updated_e = doc.NewElement("updated");
      updated_e->SetText(durStr.c_str());
      root->InsertEndChild(updated_e);

      for(auto &i : posts) {
        auto entry_e = doc.NewElement("entry");
        
        auto entry_id_e = doc.NewElement("id");
        entry_id_e->SetText(("c3blog://post/" + url + std::to_string(i.post_time)).c_str());
        entry_e->InsertEndChild(entry_id_e);

        auto entry_title_e = doc.NewElement("title");
        entry_title_e->SetText(i.topic.c_str());
        entry_e->InsertEndChild(entry_title_e);

        auto entry_content_e = doc.NewElement("content");
        entry_content_e->SetText(markdown(i.content).c_str());
        entry_content_e->SetAttribute("type", "html");
        entry_e->InsertEndChild(entry_content_e);

        auto entry_updated_e = doc.NewElement("updated");
        entry_updated_e->SetText(generate_rfc3999(i.update_time).c_str());
        entry_e->InsertEndChild(entry_updated_e);

        auto entry_published_e = doc.NewElement("published");
        entry_published_e->SetText(generate_rfc3999(i.update_time).c_str());
        entry_e->InsertEndChild(entry_published_e);

        auto entry_link_e = doc.NewElement("link");
        entry_link_e->SetAttribute("href", (url + "/" + i.url).c_str());
        entry_link_e->SetAttribute("rel", "alternate");
        entry_e->InsertEndChild(entry_link_e);

        auto entry_author_e = doc.NewElement("author");
        auto entry_author_name_e = doc.NewElement("name");

        try {
          User author = get_user(i.uident);
          auto entry_author_email_e = doc.NewElement("email");
          entry_author_name_e->SetText(author.name.c_str());
          entry_author_email_e->SetText(author.email.c_str());

          entry_author_e->InsertEndChild(entry_author_name_e);
          entry_author_e->InsertEndChild(entry_author_email_e);
        } catch(StorageExcept e) {
          entry_author_name_e->SetText("Anonymous");
          entry_author_e->InsertEndChild(entry_author_name_e);
        }

        entry_e->InsertEndChild(entry_author_e);

        root->InsertEndChild(entry_e);
      }

      doc.InsertEndChild(dec);
      doc.InsertEndChild(root);

      tinyxml2::XMLPrinter printer(NULL, true, 0);
      doc.Print(&printer);
      atom_str = printer.CStr();

      atom_valid = true;
    }

    std::string fetchAtom(void) {
      if(!atom_valid) updateAtom();

      return atom_str;
    }

    void updateSitemap(void) {
      bool dummy_hasNext;
      uint64_t dummy_total;
      auto posts = list_posts(0, -1, dummy_hasNext, dummy_total);

      tinyxml2::XMLDocument doc;

      auto dec = doc.NewDeclaration();
      auto root = doc.NewElement("urlset");
      root->SetAttribute("xmlns", "http://www.sitemaps.org/schemas/sitemap/0.9");

      for(auto &p : posts) {
        auto url_e = doc.NewElement("url");

        auto loc_e = doc.NewElement("loc");
        loc_e->SetText((url + p.url).c_str());
        url_e->InsertEndChild(loc_e);

        auto lastmod_e = doc.NewElement("lastmod");
        lastmod_e->SetText(generate_rfc3999(p.update_time).c_str());
        url_e->InsertEndChild(lastmod_e);

        root->InsertEndChild(url_e);
      }

      doc.InsertEndChild(dec);
      doc.InsertEndChild(root);

      tinyxml2::XMLPrinter printer(NULL, true, 0);
      doc.Print(&printer);
      sitemap_str = printer.CStr();

      sitemap_valid = true;
    }

    std::string fetchSitemap(void) {
      if(!sitemap_valid) updateSitemap();

      return sitemap_str;
    }
  }
}
