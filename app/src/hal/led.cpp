#include "led.hpp"

#include <zephyr/device.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(led_cpp, LOG_LEVEL_INF);

#define STRIP_NODE DT_ALIAS(led_strip)
#define STRIP_NUM_PIXELS 1
#define LED_BRIGHTNESS 0x20

static const struct device* const strip = DEVICE_DT_GET(STRIP_NODE);
static struct led_rgb pixels[STRIP_NUM_PIXELS];

int Led::init(void) {
  if (!device_is_ready(strip)) {
    LOG_ERR("LED strip device %s is not ready", strip->name);
    return -ENODEV;
  }
  LOG_INF("LED strip device %s ready", strip->name);
  return 0;
}

int Led::set_color(uint8_t r, uint8_t g, uint8_t b) {
  if (!device_is_ready(strip)) {
    return -ENODEV;
  }

  pixels[0].r = r;
  pixels[0].g = g;
  pixels[0].b = b;

  int rc = led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);
  if (rc) {
    LOG_ERR("couldn't update strip: %d", rc);
    return rc;
  }
  return 0;
}
