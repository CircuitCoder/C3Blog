#pragma once

#include <string>
#include "config.h"

namespace C3 {
  namespace Feed {
    void setup(const Config &c);
    void invalidate(void);
    void updateAtom(void);
    std::string fetchAtom(void);
    void updateSitemap(void);
    std::string fetchSitemap(void);
  }
}
