#include "app.h"

#include <lvgl.h>
#include <stdint.h>
#include <sys/_stdint.h>
#include <zephyr/drivers/display.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "app/model.h"
#include "app/screen.h"
#include "app/screens/noti_screen.h"
#include "app/screens/watchface_screen.h"
#include "display/lv_display.h"
#include "hal/ancs_client.h"
#include "misc/lv_color.h"
#include "rtc.h"
#include "ui/ui.h"

LOG_MODULE_REGISTER(ui_module, LOG_LEVEL_DBG);

#define UI_LCD_WIDTH 176
#define UI_LCD_HEIGHT 176

#define UI_DRAW_BUF_PIXELS (UI_LCD_WIDTH * UI_LCD_HEIGHT)
#define UI_DRAW_BUF_BYTES (UI_DRAW_BUF_PIXELS * LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_RGB565))

static uint8_t draw_buf_mem[UI_DRAW_BUF_BYTES] __aligned(16);

static lv_display_t* disp;
static screen_t* current_screen = NULL;

static void ui_display_flush_cb(lv_display_t* display, const lv_area_t* area, uint8_t* px_map) {
  const struct device* display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

  if (!device_is_ready(display_dev)) {
    LOG_ERR("Display device not ready");
    return;
  }

  uint16_t w = area->x2 - area->x1 + 1;
  uint16_t h = area->y2 - area->y1 + 1;

  struct display_buffer_descriptor desc = {
      .buf_size = w * h * 2,
      .width = w,
      .height = h,
      .pitch = w,
  };

  display_write(display_dev, area->x1, area->y1, &desc, px_map);
  lv_display_flush_ready(display);
}

void app_switch_screen(screen_t* screen) {
  if (screen == NULL || screen == current_screen) {
    return;
  }
  current_screen = screen;
  if (current_screen->load) {
    current_screen->load();
  }
}

int app_init(void) {
  const struct device* display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

  if (!device_is_ready(display_dev)) {
    LOG_ERR("Display device not ready");
    return -ENODEV;
  }

  display_blanking_off(display_dev);

#if !IS_ENABLED(CONFIG_LV_Z_AUTO_INIT)
  lv_init();
#endif
  memset(draw_buf_mem, 0, sizeof(draw_buf_mem));
  disp = lv_display_create(UI_LCD_WIDTH, UI_LCD_HEIGHT);
  LOG_INF("LVGL display created at %p", disp);
  lv_display_delete(lv_display_get_default());
  lv_display_set_default(disp);
  lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
  lv_display_set_flush_cb(disp, ui_display_flush_cb);
  lv_display_set_buffers(disp, draw_buf_mem, NULL, UI_DRAW_BUF_BYTES, LV_DISPLAY_RENDER_MODE_FULL);

  ui_init();
  LOG_INF("UI init done");

  // Initialize screens
  watchface_screen.init();
  noti_screen.init();

  // Load default screen
  app_switch_screen(&watchface_screen);

  return 0;
}

K_MSGQ_DEFINE(app_msgq, sizeof(app_event_t), 10, 4);

int app_event_post(app_event_t* event) {
  LOG_INF("Posting event type: %u", event->type);
  return k_msgq_put(&app_msgq, event, K_NO_WAIT);
}

uint32_t app_task_handler(void) {
  uint32_t sleep = 1;
  while (1) {
    app_event_t event;
    while (k_msgq_get(&app_msgq, &event, K_MSEC(sleep)) == 0) {
      LOG_INF("Handling event type: %u", event.type);
      if (event.type == APP_EVENT_BLE_ANCS) {
        // App handles notification management first
        if (event.ptr) {
          model_add_notification((ancs_noti_info_t*)event.ptr);
          k_free(event.ptr);

          model_dump_notifications();
        }
      }

      if (current_screen && current_screen->handle_event) {
        current_screen->handle_event(&event);
      }
    }
    sleep = lv_timer_handler();
    if (sleep > 1000) {
      sleep = 1000;
    }
  }
}
