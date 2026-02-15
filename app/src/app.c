#include "app.h"

#include <lvgl.h>
#include <stdint.h>
#include <sys/_stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "display/lv_display.h"
#include "driver/LPM013M126A.h"
#include "misc/lv_color.h"
#include "rtc.h"
#include "ui/ui.h"

LOG_MODULE_REGISTER(ui_module, LOG_LEVEL_DBG);

#define UI_LCD_WIDTH LCD_DEVICE_WIDTH
#define UI_LCD_HEIGHT LCD_DEVICE_HEIGHT

#define UI_DRAW_BUF_PIXELS (UI_LCD_WIDTH * UI_LCD_HEIGHT)
#define UI_DRAW_BUF_BYTES (UI_DRAW_BUF_PIXELS * LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_RGB565))

static uint8_t draw_buf_mem[UI_DRAW_BUF_BYTES] __aligned(16);

static lv_display_t* disp;

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

int app_init(void) {
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
  disp = lv_display_create(UI_LCD_WIDTH, UI_LCD_HEIGHT);
  LOG_INF("LVGL display created at %p", disp);
  lv_display_delete(lv_display_get_default());
  lv_display_set_default(disp);
  lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
  lv_display_set_flush_cb(disp, ui_display_flush_cb);
  lv_display_set_buffers(disp, draw_buf_mem, NULL, UI_DRAW_BUF_BYTES, LV_DISPLAY_RENDER_MODE_FULL);

  ui_init();
  LOG_INF("UI init done");
  return 0;
}

K_MSGQ_DEFINE(app_msgq, sizeof(app_event_t), 10, 4);

static uint32_t current_brightness = 0;

int app_event_post(app_event_t* event) {
  LOG_INF("Posting event type: %u", event->type);
  return k_msgq_put(&app_msgq, event, K_NO_WAIT);
}

typedef void (*event_handler_fn)(app_event_t*);

static void event_handle_button(app_event_t* event) {
  uint32_t button_idx = event->value;
  LOG_INF("Button %d event", button_idx);
  if (button_idx == 0) {
    // Button 1: Increase brightness
    if (current_brightness < 100) {
      current_brightness += 10;
    }
  } else if (button_idx == 2) {
    // Button 3: Decrease brightness
    if (current_brightness > 0) {
      current_brightness -= 10;
    }
  }
  LOG_INF("Setting brightness to %d%%", current_brightness);
  cmlcd_backlight_set(100 - current_brightness);
}

static void event_handle_rtc_alarm(app_event_t* event) {
  (void)event;
  struct rtc_time time;
  static const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

  if (rtc_time_get(&time) != 0) {
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

static void event_handle_battery(app_event_t* event) {
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

static void event_handle_ble_cts(app_event_t* event) {
  if (event->len == sizeof(struct rtc_time) && event->ptr) {
    rtc_time_set((struct rtc_time*)event->ptr);
    // Update display immediately with new time
    event_handle_rtc_alarm(event);
  }
  if (event->ptr) {
    k_free(event->ptr);
  }
}

static void event_handle_ble_ancs(app_event_t* event) {
  if (event->ptr) {
    k_free(event->ptr);
  }
}

typedef struct {
  uint32_t event_id;
  event_handler_fn handler;
} event_handler_map_t;

static const event_handler_map_t event_handler_map[] = {
    {APP_EVENT_BUTTON, event_handle_button},     {APP_EVENT_RTC_ALARM, event_handle_rtc_alarm},
    {APP_EVENT_BATTERY, event_handle_battery},   {APP_EVENT_BLE_CTS, event_handle_ble_cts},
    {APP_EVENT_BLE_ANCS, event_handle_ble_ancs},
};

static event_handler_fn get_event_handler(uint32_t event_id) {
  for (size_t i = 0; i < sizeof(event_handler_map) / sizeof(event_handler_map[0]); ++i) {
    if (event_handler_map[i].event_id == event_id) {
      return event_handler_map[i].handler;
    }
  }
  return NULL;
}

uint32_t app_task_handler(void) {
  uint32_t sleep = 1;
  while (1) {
    app_event_t event;
    while (k_msgq_get(&app_msgq, &event, K_MSEC(sleep)) == 0) {
      event_handler_fn handler = get_event_handler(event.type);
      if (handler) {
        handler(&event);
      }
    }
    sleep = lv_timer_handler();
    if (sleep > 1000) {
      sleep = 1000;
    }
  }
}
