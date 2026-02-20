#include "watchface_screen.hpp"

#include <lvgl.h>
#include <zephyr/logging/log.h>

#include "../../hal/rtc.hpp"
#include "../app.hpp"
#include "../model.hpp"
#include "../modes.hpp"
#include "../ui/ui.h"
#include "noti_screen.hpp"

LOG_MODULE_REGISTER(watchface_screen_cpp);

void WatchfaceScreen::handle_rtc_alarm(app_event_t* event) {
  (void)event;
  struct rtc_time time;
  static const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

  if (Rtc::instance().time_get(&time) != 0) {
    LOG_ERR("Failed to get RTC time");
    return;
  }

  lv_label_set_text_fmt(ui_HourLabel, "%02d", time.tm_hour);
  lv_label_set_text_fmt(ui_MinuteLabel, "%02d", time.tm_min);
  lv_label_set_text_fmt(ui_Label6, "%s %d, %d", months[time.tm_mon], time.tm_mday, time.tm_year + 1900);

  // Set AM/PM icon
  if (time.tm_hour < 12) {
    lv_img_set_src(ui_Image2, &ui_img_sun_png);
  } else {
    lv_img_set_src(ui_Image2, &ui_img_moon_png);
  }

  // Date row update only
  static lv_obj_t** const date_labels[7] = {&ui_day1, &ui_day2, &ui_day3, &ui_day4, &ui_day5, &ui_day6, &ui_day7};
  // tm_wday: 0=Sun, 6=Sat; shift to 0=Mon
  int wday = time.tm_wday == 0 ? 6 : time.tm_wday - 1;
  int today = time.tm_mday;
  struct rtc_time t = time;
  int delta = wday;
  t.tm_mday -= delta;
  for (int i = 0; i < 7; ++i) {
    int d = t.tm_mday + i;
    lv_label_set_text_fmt(*date_labels[i], "%02d", d);
    // Highlight current day with round rect border only, not filled
    if (d == today) {
      lv_obj_set_style_border_color(*date_labels[i], lv_color_black(), 0);
      lv_obj_set_style_border_width(*date_labels[i], 2, 0);
      // Rectangle border
      lv_obj_set_style_radius(*date_labels[i], 1, 0);
    } else {
      lv_obj_set_style_border_width(*date_labels[i], 0, 0);
    }
  }
}

void WatchfaceScreen::handle_battery(app_event_t* event) {
  uint32_t percent = event->value;
  int battery_index = percent / 20;
  if (battery_index > 5) {
    battery_index = 5;
  }
  const static lv_image_dsc_t* battery_icons[] = {
      &ui_img_battery_status_0_png,  // empty
      &ui_img_battery_status_1_png,  // 20%
      &ui_img_battery_status_2_png,  // 40%
      &ui_img_battery_status_3_png,  // 60%
      &ui_img_battery_status_4_png,  // 80%
      &ui_img_battery_status_5_png,  // full
  };
  LOG_INF("Battery percent: %u, index: %d", percent, battery_index);
  lv_img_set_src(ui_batteryIcon, battery_icons[battery_index]);
  lv_label_set_text_fmt(ui_Label5, "%u%%", percent);
}

void WatchfaceScreen::handle_button(app_event_t* event) {
  uint32_t button_idx = (uint32_t)event->value;
  LOG_INF("Button %d event", button_idx);
  switch (button_idx) {
    case 0: {
      uint8_t brightness = Modes::instance().get_active_brightness();
      if (brightness < 100) {
        brightness += 10;
      }
      Modes::instance().set_active_brightness(brightness);
      break;
    }
    case 1:
      // Button 1: Switch to notification screen
      App::instance().switch_screen(&NotiScreen::instance());
      break;
    case 2: {
      uint8_t brightness = Modes::instance().get_active_brightness();
      if (brightness > 0) {
        brightness -= 10;
      }
      Modes::instance().set_active_brightness(brightness);
      break;
    }
    default:
      LOG_WRN("Unhandled button index: %d", button_idx);
      break;
  }
}

void WatchfaceScreen::init() { lv_label_set_text_fmt(ui_numNoti, "%u", Model::instance().get_notification_count()); }

void WatchfaceScreen::load() {
  lv_screen_load(ui_Screen1);
  // Trigger initial update
  handle_rtc_alarm(nullptr);
}

void WatchfaceScreen::handle_event(app_event_t* event) {
  switch (event->type) {
    case APP_EVENT_RTC_ALARM:
      handle_rtc_alarm(event);
      break;
    case APP_EVENT_BATTERY:
      handle_battery(event);
      break;
    case APP_EVENT_BUTTON:
      handle_button(event);
      break;
    case APP_EVENT_BLE_ANCS:
      // Update notification count
      const uint32_t count = Model::instance().get_notification_count();
      LOG_INF("Notification count updated: %u", count);
      lv_label_set_text_fmt(ui_numNoti, "%u", count);
      break;
    default:
      break;
  }
}
