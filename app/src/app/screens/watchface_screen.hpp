#ifndef WATCHFACE_SCREEN_HPP
#define WATCHFACE_SCREEN_HPP

#include "../screen.hpp"

class WatchfaceScreen : public Screen {
 public:
  static WatchfaceScreen& instance() {
    static WatchfaceScreen inst;
    return inst;
  }

  void init() override;
  void handle_event(app_event_t* event) override;
  void load() override;

 private:
  WatchfaceScreen() = default;

  void handle_rtc_alarm(app_event_t* event);
  void handle_battery(app_event_t* event);
  void handle_button(app_event_t* event);
};

#endif  // WATCHFACE_SCREEN_HPP
