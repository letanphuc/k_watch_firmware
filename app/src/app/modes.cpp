#include "modes.hpp"

#include <zephyr/logging/log.h>

#include "../app.hpp"
#include "../driver/LPM013M126A.h"

LOG_MODULE_REGISTER(modes_cpp, LOG_LEVEL_INF);

#define MODE_TIMEOUT_MS 20000

void Modes::timer_expiry_wrapper(struct k_timer* timer_id) { Modes::instance().handle_timeout_event(); }

// I need to add handle_timeout_event to post the event
// Wait, the original modes.c posted an event to app.
// I need to update app.hpp first or just use app_event_post if it's still accessible.
// Since App will be a class, I'll use its instance.

void Modes::init() {
  k_timer_init(
      &mode_timer_,
      [](struct k_timer* timer_id) {
        app_event_t event = {
            .type = APP_EVENT_MODE_TIMEOUT,
        };
        App::instance().event_post(&event);
      },
      NULL);

  // Start in active mode
  current_mode_ = APP_MODE_ACTIVE;
  cmlcd_backlight_set(active_brightness_);
  k_timer_start(&mode_timer_, K_MSEC(MODE_TIMEOUT_MS), K_NO_WAIT);
  LOG_INF("Modes initialized, default brightness: %d%%", active_brightness_);
}

void Modes::activity_detected() {
  if (current_mode_ == APP_MODE_AMBIENT) {
    LOG_INF("Activity detected: Entering ACTIVE mode");
    current_mode_ = APP_MODE_ACTIVE;
    cmlcd_backlight_set(active_brightness_);
  }
  // Reset timer
  k_timer_start(&mode_timer_, K_MSEC(MODE_TIMEOUT_MS), K_NO_WAIT);
}

void Modes::set_active_brightness(uint8_t brightness) {
  if (brightness > 100) brightness = 100;
  active_brightness_ = brightness;
  LOG_INF("Active brightness set to %d%%", active_brightness_);
  if (current_mode_ == APP_MODE_ACTIVE) {
    cmlcd_backlight_set(active_brightness_);
  }
}

void Modes::handle_timeout() {
  if (current_mode_ == APP_MODE_ACTIVE) {
    LOG_INF("Timeout reached: Entering AMBIENT mode");
    current_mode_ = APP_MODE_AMBIENT;
    cmlcd_backlight_set(0);
  }
}
