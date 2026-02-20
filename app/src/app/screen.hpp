#ifndef APP_SCREEN_HPP
#define APP_SCREEN_HPP

#include <stdint.h>

#include "../event.h"

class Screen {
 public:
  virtual ~Screen() = default;
  virtual void init() {}
  virtual void handle_event(app_event_t* event) = 0;
  virtual void load() = 0;
};

#endif  // APP_SCREEN_HPP
