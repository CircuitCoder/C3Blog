#include <string>

#include "config.h"

namespace C3 {
  namespace Feed {
    void setup(Config &c);
    void update(void);
    std::string fetch(void);
  }
}
