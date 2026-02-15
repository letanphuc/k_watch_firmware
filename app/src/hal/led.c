#include <zephyr/device.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(led);

#define STRIP_NODE DT_ALIAS(led_strip)
#define STRIP_NUM_PIXELS 1

#define RGB(_r, _g, _b) {.r = (_r), .g = (_g), .b = (_b)}
#define LED_BRIGHTNESS 0x20

static const struct device* const strip = DEVICE_DT_GET(STRIP_NODE);
static struct led_rgb pixels[STRIP_NUM_PIXELS];

static const struct led_rgb colors[] = {
    RGB(LED_BRIGHTNESS, 0x00, 0x00), /* red */
    RGB(0x00, LED_BRIGHTNESS, 0x00), /* green */
    RGB(0x00, 0x00, LED_BRIGHTNESS), /* blue */
};

int led_init(void) {
  if (!device_is_ready(strip)) {
    LOG_ERR("LED strip device %s is not ready", strip->name);
    return -ENODEV;
  }
  LOG_INF("LED strip device %s ready", strip->name);
  return 0;
}

int led_set_color(uint8_t r, uint8_t g, uint8_t b) {
  if (!device_is_ready(strip)) {
    return -ENODEV;
  }

  struct led_rgb color = RGB(r, g, b);
  pixels[0] = color;

  int rc = led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);
  if (rc) {
    LOG_ERR("couldn't update strip: %d", rc);
    return rc;
  }
  return 0;
}
