#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/poweroff.h>

#include "app.hpp"
#include "hal/ble.hpp"
#include "hal/buttons.hpp"
#include "hal/led.hpp"
#include "hal/power.hpp"
#include "hal/rtc.hpp"

LOG_MODULE_REGISTER(main_cpp);

static void led_test(void) {
  LOG_INF("Testing LEDs...");
  for (int i = 0; i < 3; ++i) {
    Led::instance().set_color(0x20, 0x00, 0x00); /* red */
    k_msleep(200);
    Led::instance().set_color(0x00, 0x20, 0x00); /* green */
    k_msleep(200);
    Led::instance().set_color(0x00, 0x00, 0x20); /* blue */
    k_msleep(200);
  }
  Led::instance().set_color(0x00, 0x00, 0x00); /* off */
}

int main(void) {
  LOG_INF("K Watch starting (C++)");

  Buttons::instance().init();
  Rtc::instance().init();
  Led::instance().init();
  Power::instance().init();

  led_test();

  if (Ble::instance().init() < 0) {
    LOG_ERR("BLE init failed");
  }

  if (App::instance().init() < 0) {
    LOG_ERR("UI init failed");
  }

  if (Rtc::instance().minute_alarm_enable() < 0) {
    LOG_ERR("RTC minute alarm init failed");
  }

  while (1) {
    App::instance().task_handler();
  }
  return 0;
}
