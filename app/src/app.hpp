#ifndef APP_HPP
#define APP_HPP

#include <stdint.h>

#include "app/screen.hpp"
#include "event.h"

class App {
 public:
  static App& instance() {
    static App inst;
    return inst;
  }

  int init();
  uint32_t task_handler();
  int event_post(app_event_t* event);
  void switch_screen(Screen* screen);

 private:
  App() : current_screen_(nullptr) {}

  Screen* current_screen_;
  struct k_msgq msgq_;
};

#endif /* APP_HPP */
