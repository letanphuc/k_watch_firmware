#include "noti_screen.h"

#include <lvgl.h>
#include <zephyr/logging/log.h>

#include "../../hal/ancs_client.h"
#include "../../ui/ui.h"
#include "../app.h"
#include "../model.h"
#include "watchface_screen.h"

LOG_MODULE_DECLARE(ui_module);

static uint8_t current_noti_index = 0;

static void noti_display_current(void) {
  uint8_t count = model_get_notification_count();
  if (count == 0) {
    lv_label_set_text(ui_title, "No Notifications");
    lv_label_set_text(ui_content, "");
    return;
  }

  if (current_noti_index >= count) {
    current_noti_index = 0;
  }

  const ancs_noti_info_t* info = model_get_notification(current_noti_index);
  if (info) {
    lv_label_set_text(ui_title, info->title);
    lv_label_set_text(ui_content, info->message);
  }
  lv_label_set_text_fmt(ui_Label3, "%d/%d", current_noti_index + 1, count);
}

static void noti_handle_button(app_event_t* event) {
  uint32_t button_idx = (uint32_t)event->value;
  LOG_INF("Noti Screen: Button %d event", button_idx);

  if (button_idx == 1) {  // Button 1
    uint8_t count = model_get_notification_count();
    if (count > 0) {
      current_noti_index = (current_noti_index + 1) % count;
      noti_display_current();
    }
  } else if (button_idx == 3) {  // Button 3
    app_switch_screen(&watchface_screen);
  }
}

static void noti_init(void) {}

static void noti_load(void) {
  model_dump_notifications();
  lv_screen_load(ui_Screen2);
  current_noti_index = 0;
  noti_display_current();
}

static void noti_handle_event(app_event_t* event) {
  switch (event->type) {
    case APP_EVENT_BUTTON:
      noti_handle_button(event);
      break;
    default:
      break;
  }
}

screen_t noti_screen = {
    .init = noti_init,
    .handle_event = noti_handle_event,
    .load = noti_load,
};
