#include "display.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "driver/LPM013M126A.h"
#include "image_176x176_4bpp.h"

LOG_MODULE_REGISTER(display_module);

static void display_draw_4bpp(int x0, int y0, int width, int height, const uint8_t *buf) {
  int bytes_per_row = width / 2;

  for (int y = 0; y < height; y++) {
    for (int xb = 0; xb < bytes_per_row; xb++) {
      uint8_t packed = buf[(y * bytes_per_row) + xb];
      uint8_t px0 = (packed >> 4) & 0x0F;
      uint8_t px1 = packed & 0x0F;

      cmlcd_draw_pixel(x0 + (xb * 2), y0 + y, px0);
      cmlcd_draw_pixel(x0 + (xb * 2) + 1, y0 + y, px1);
    }
  }
}

int display_test(void) {
  int ret = cmlcd_init();
  if (ret < 0) {
    LOG_ERR("Display init failed: %d", ret);
    return ret;
  }

  LOG_INF("Display test start");

  cmlcd_set_blink_mode(LCD_BLINKMODE_NONE);

  display_draw_4bpp(0, 0, IMAGE_176X176_4BPP_WIDTH, IMAGE_176X176_4BPP_HEIGHT, image_176x176_4bpp);
  cmlcd_refresh();

  cmlcd_backlight_set(100);

  LOG_INF("Display test done");
  return 0;
}
