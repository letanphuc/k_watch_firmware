#include "ui.h"

#include <lvgl.h>
#include <sys/_stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "display/lv_display.h"
#include "driver/LPM013M126A.h"
#include "misc/lv_color.h"
#include "watchface.h"

LOG_MODULE_REGISTER(ui_module, LOG_LEVEL_DBG);

#define UI_LCD_WIDTH LCD_DEVICE_WIDTH
#define UI_LCD_HEIGHT LCD_DEVICE_HEIGHT

#if LVGL_VERSION_MAJOR != 9
#error "ui.c supports LVGL v9 only"
#endif

#if LVGL_VERSION_MINOR != 3
#error "ui.c supports LVGL v9.3 only"
#endif

#if LV_COLOR_DEPTH != 16
#error "ui.c requires LV_COLOR_DEPTH=16 (RGB565)"
#endif

#define UI_DRAW_BUF_PIXELS (UI_LCD_WIDTH * UI_LCD_HEIGHT)
#define UI_DRAW_BUF_BYTES (UI_DRAW_BUF_PIXELS * LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_RGB565))

static uint8_t draw_buf_mem[UI_DRAW_BUF_BYTES] __aligned(16);

static lv_display_t *disp;

struct k_sem lvgl_refresh_sem;

static uint8_t rgb565_to_lcd4(uint16_t rgb565) {
  uint8_t r5 = (rgb565 >> 11) & 0x1F;
  uint8_t g6 = (rgb565 >> 5) & 0x3F;
  uint8_t b5 = rgb565 & 0x1F;
  uint8_t r1 = (r5 >= 16U) ? 1U : 0U;
  uint8_t g1 = (g6 >= 32U) ? 1U : 0U;
  uint8_t b1 = (b5 >= 16U) ? 1U : 0U;

  return (uint8_t)((((r1 << 2) | (g1 << 1) | b1) << 1) & 0x0F);
}

static void ui_display_flush_cb(lv_display_t *display, const lv_area_t *area, uint8_t *px_map) {
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

void ui_watchface_update(void) { watchface_update(); }

int ui_init(void) {
  int ret;

  k_sem_init(&lvgl_refresh_sem, 0, 1);

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
  lv_display_set_buffers(disp, draw_buf_mem, NULL, UI_DRAW_BUF_BYTES, LV_DISPLAY_RENDER_MODE_PARTIAL);

  watchface_init();
  watchface_update();
  LOG_INF("UI init done");
  return 0;
}

uint32_t ui_task_handler(void) { return lv_timer_handler(); }
