#include "modes.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "../driver/LPM013M126A.h"
#include "../event.h"

LOG_MODULE_REGISTER(modes, LOG_LEVEL_INF);

#define MODE_TIMEOUT_MS 20000

static app_mode_t current_mode = APP_MODE_ACTIVE;
static uint8_t active_brightness = 10;
static struct k_timer mode_timer;

static void mode_timer_expiry_fn(struct k_timer* timer_id) {
  app_event_t event = {
      .type = APP_EVENT_MODE_TIMEOUT,
  };
  event_post(&event);
}

void modes_init(void) {
  k_timer_init(&mode_timer, mode_timer_expiry_fn, NULL);
  // Start in active mode
  current_mode = APP_MODE_ACTIVE;
  cmlcd_backlight_set(active_brightness);
  k_timer_start(&mode_timer, K_MSEC(MODE_TIMEOUT_MS), K_NO_WAIT);
  LOG_INF("Modes initialized, default brightness: %d%%", active_brightness);
}

void modes_activity_detected(void) {
  if (current_mode == APP_MODE_AMBIENT) {
    LOG_INF("Activity detected: Entering ACTIVE mode");
    current_mode = APP_MODE_ACTIVE;
    cmlcd_backlight_set(active_brightness);
  }
  // Reset timer
  k_timer_start(&mode_timer, K_MSEC(MODE_TIMEOUT_MS), K_NO_WAIT);
}

void modes_set_active_brightness(uint8_t brightness) {
  if (brightness > 100) brightness = 100;
  active_brightness = brightness;
  LOG_INF("Active brightness set to %d%%", active_brightness);
  if (current_mode == APP_MODE_ACTIVE) {
    cmlcd_backlight_set(active_brightness);
  }
}

uint8_t modes_get_active_brightness(void) { return active_brightness; }

void modes_handle_timeout(void) {
  if (current_mode == APP_MODE_ACTIVE) {
    LOG_INF("Timeout reached: Entering AMBIENT mode");
    current_mode = APP_MODE_AMBIENT;
    cmlcd_backlight_set(0);
  }
}
