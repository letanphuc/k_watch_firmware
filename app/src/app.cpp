#include "app.hpp"

#include <lvgl.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "app/model.hpp"
#include "app/modes.hpp"
#include "app/screen.hpp"
#include "app/screens/noti_screen.hpp"
#include "app/screens/watchface_screen.hpp"
#include "display/lv_display.h"
#include "hal/ancs_client.hpp"
#include "hal/rtc.hpp"
#include "ui/ui.h"

LOG_MODULE_REGISTER(ui_module_cpp, LOG_LEVEL_DBG);

#define UI_LCD_WIDTH LCD_DEVICE_WIDTH
#define UI_LCD_HEIGHT LCD_DEVICE_HEIGHT

#define UI_DRAW_BUF_PIXELS (UI_LCD_WIDTH * UI_LCD_HEIGHT)
#define UI_DRAW_BUF_BYTES (UI_DRAW_BUF_PIXELS * LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_RGB565))

static uint8_t draw_buf_mem[UI_DRAW_BUF_BYTES] __aligned(16);
static lv_display_t* disp_ptr;

static uint8_t rgb565_to_lcd4(uint16_t rgb565) {
  uint8_t r5 = (rgb565 >> 11) & 0x1F;
  uint8_t g6 = (rgb565 >> 5) & 0x3F;
  uint8_t b5 = rgb565 & 0x1F;
  uint8_t r1 = (r5 >= 16U) ? 1U : 0U;
  uint8_t g1 = (g6 >= 32U) ? 1U : 0U;
  uint8_t b1 = (b5 >= 16U) ? 1U : 0U;

  return (uint8_t)((((r1 << 2) | (g1 << 1) | b1) << 1) & 0x0F);
}

static void ui_display_flush_cb(lv_display_t* display, const lv_area_t* area, uint8_t* px_map) {
  for (int y = area->y1; y <= area->y2; y++) {
    for (int x = area->x1; x <= area->x2; x++) {
      uint16_t rgb565 = (uint16_t)px_map[0] | ((uint16_t)px_map[1] << 8);
      cmlcd_draw_pixel((int16_t)x, (int16_t)y, rgb565_to_lcd4(rgb565));
      px_map += 2;
    }
  }
  LOG_DBG("Flushed area x1:%d y1:%d x2:%d y2:%d", area->x1, area->y1, area->x2, area->y2);
  cmlcd_refresh();
  lv_display_flush_ready(display);
}

void App::switch_screen(Screen* screen) {
  if (screen == nullptr || screen == current_screen_) {
    return;
  }
  current_screen_ = screen;
  current_screen_->load();
}

K_MSGQ_DEFINE(app_msgq_cpp, sizeof(app_event_t), 10, 4);

int App::init() {
  int ret;

  ret = cmlcd_init();
  if (ret < 0) {
    LOG_ERR("Display init failed: %d", ret);
    return ret;
  }

  cmlcd_set_blink_mode(LCD_BLINKMODE_NONE);
  cmlcd_cls();
  cmlcd_refresh();

#if !IS_ENABLED(CONFIG_LV_Z_AUTO_INIT)
  lv_init();
#endif
  memset(draw_buf_mem, 0, sizeof(draw_buf_mem));
  disp_ptr = lv_display_create(UI_LCD_WIDTH, UI_LCD_HEIGHT);
  LOG_INF("LVGL display created at %p", disp_ptr);
  lv_display_delete(lv_display_get_default());
  lv_display_set_default(disp_ptr);
  lv_display_set_color_format(disp_ptr, LV_COLOR_FORMAT_RGB565);
  lv_display_set_flush_cb(disp_ptr, ui_display_flush_cb);
  lv_display_set_buffers(disp_ptr, draw_buf_mem, NULL, UI_DRAW_BUF_BYTES, LV_DISPLAY_RENDER_MODE_FULL);

  ui_init();
  LOG_INF("UI init done");

  // Initialize screens
  WatchfaceScreen::instance().init();
  NotiScreen::instance().init();

  // Initialize modes
  Modes::instance().init();

  // Load default screen
  switch_screen(&WatchfaceScreen::instance());

  return 0;
}

int App::event_post(app_event_t* event) {
  LOG_INF("Posting event type: %u", event->type);
  return k_msgq_put(&app_msgq_cpp, event, K_NO_WAIT);
}

uint32_t App::task_handler() {
  uint32_t sleep = 1;
  while (1) {
    app_event_t event;
    while (k_msgq_get(&app_msgq_cpp, &event, K_MSEC(sleep)) == 0) {
      LOG_INF("Handling event type: %u", event.type);
      if (event.type == APP_EVENT_BLE_ANCS) {
        // App handles notification management first
        if (event.ptr) {
          Model::instance().add_notification((ancs_noti_info_t*)event.ptr);
          k_free(event.ptr);
          Model::instance().dump_notifications();
        }
      } else if (event.type == APP_EVENT_BUTTON) {
        Modes::instance().activity_detected();
      } else if (event.type == APP_EVENT_MODE_TIMEOUT) {
        Modes::instance().handle_timeout();
      }

      if (current_screen_) {
        current_screen_->handle_event(&event);
      }
    }
    sleep = lv_timer_handler();
    if (sleep > 1000) {
      sleep = 1000;
    }
  }
}
