#pragma once

#include <string>
#include "config.h"

namespace C3 {
  namespace Feed {
    void setup(const Config &c);
    void invalidate(void);
    void update(void);
    std::string fetch(void);
  }
}
