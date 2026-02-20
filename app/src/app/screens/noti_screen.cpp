#include "noti_screen.hpp"

#include <lvgl.h>
#include <zephyr/logging/log.h>

#include "../../hal/ancs_client.hpp"
#include "../../ui/ui.h"
#include "../app.hpp"
#include "../model.hpp"
#include "watchface_screen.hpp"

LOG_MODULE_DECLARE(ui_module);

void NotiScreen::display_current() {
  uint8_t count = Model::instance().get_notification_count();
  if (count == 0) {
    lv_label_set_text(ui_title, "No Notifications");
    lv_label_set_text(ui_content, "");
    return;
  }

  if (current_noti_index_ >= count) {
    current_noti_index_ = 0;
  }

  const ancs_noti_info_t* info = Model::instance().get_notification(current_noti_index_);
  if (info) {
    lv_label_set_text(ui_title, info->title);
    lv_label_set_text(ui_content, info->message);
  }
  lv_label_set_text_fmt(ui_Label3, "%d/%d", current_noti_index_ + 1, count);
}

void NotiScreen::handle_button(app_event_t* event) {
  uint32_t button_idx = (uint32_t)event->value;
  LOG_INF("Noti Screen: Button %d event", button_idx);

  if (button_idx == 1) {  // Button 1
    uint8_t count = Model::instance().get_notification_count();
    if (count > 0) {
      current_noti_index_ = (current_noti_index_ + 1) % count;
      display_current();
    }
  } else if (button_idx == 3) {  // Button 3
    App::instance().switch_screen(&WatchfaceScreen::instance());
  }
}

void NotiScreen::init() {}

void NotiScreen::load() {
  Model::instance().dump_notifications();
  lv_screen_load(ui_Screen2);
  current_noti_index_ = 0;
  display_current();
}

void NotiScreen::handle_event(app_event_t* event) {
  switch (event->type) {
    case APP_EVENT_BUTTON:
      handle_button(event);
      break;
    default:
      break;
  }
}
