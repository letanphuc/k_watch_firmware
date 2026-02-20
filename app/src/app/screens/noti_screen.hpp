#ifndef NOTI_SCREEN_HPP
#define NOTI_SCREEN_HPP

#include "../screen.hpp"

class NotiScreen : public Screen {
 public:
  static NotiScreen& instance() {
    static NotiScreen inst;
    return inst;
  }

  void init() override;
  void handle_event(app_event_t* event) override;
  void load() override;

 private:
  NotiScreen() : current_noti_index_(0) {}

  void display_current();
  void handle_button(app_event_t* event);

  uint8_t current_noti_index_;
};

#endif  // NOTI_SCREEN_HPP
