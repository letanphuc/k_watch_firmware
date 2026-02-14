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

int led_test(void) {
  if (!device_is_ready(strip)) {
    LOG_ERR("LED strip device %s is not ready", strip->name);
    return -ENODEV;
  }

  LOG_INF("LED strip device %s ready", strip->name);

  for (size_t i = 0; i < ARRAY_SIZE(colors); i++) {
    pixels[0] = colors[i];
    int rc = led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);
    if (rc) {
      LOG_ERR("couldn't update strip: %d", rc);
    }
    LOG_INF("Color %d", i);
    k_sleep(K_MSEC(1000));
  }

  /* Turn off */
  memset(pixels, 0, sizeof(pixels));
  led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);

  return 0;
}
