#ifndef MODES_HPP
#define MODES_HPP

#include <stdint.h>
#include <zephyr/kernel.h>

typedef enum {
  APP_MODE_AMBIENT,
  APP_MODE_ACTIVE,
} app_mode_t;

class Modes {
 public:
  static Modes& instance() {
    static Modes inst;
    return inst;
  }

  void init();
  void activity_detected();
  void set_active_brightness(uint8_t brightness);
  uint8_t get_active_brightness() const { return active_brightness_; }
  void handle_timeout();

 private:
  Modes() : current_mode_(APP_MODE_ACTIVE), active_brightness_(10) {}

  app_mode_t current_mode_;
  uint8_t active_brightness_;
  struct k_timer mode_timer_;

  static void timer_expiry_wrapper(struct k_timer* timer_id);
};

#endif /* MODES_HPP */
