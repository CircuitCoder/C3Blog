#pragma once

#include "handlers/post.h"
#include "handlers/util.h"
#include "handlers/account.h"
#include "handlers/feed.h"

/*
#include "handlers/index.h"
#include "handlers/comment.h"
#include "handlers/search.h"
*/

namespace C3 {
  void setup_handlers(const Config &c) {
    setup_post_handler(c);
  }
}
