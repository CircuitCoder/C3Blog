#pragma once

#include "handlers/post.h"
#include "handlers/util.h"
#include "handlers/account.h"
#include "handlers/feed.h"
#include "handlers/search.h"

/*
#include "handlers/comment.h"
*/

namespace C3 {
  void setup_handlers(const Config &c) {
    setup_post_handler(c);
    setup_account_handler(c);
    setup_search_handler(c);
  }
}
