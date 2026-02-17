/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/poweroff.h>

#include "app.h"
#include "ble.h"
#include "buttons.h"
#include "led.h"
#include "power.h"
#include "rtc.h"

LOG_MODULE_REGISTER(main);

static void led_test(void) {
  LOG_INF("Testing LEDs...");
  for (int i = 0; i < 3; ++i) {
    led_set_color(0x20, 0x00, 0x00); /* red */
    k_msleep(200);
    led_set_color(0x00, 0x20, 0x00); /* green */
    k_msleep(200);
    led_set_color(0x00, 0x00, 0x20); /* blue */
    k_msleep(200);
  }
  led_set_color(0x00, 0x00, 0x00); /* off */
}

int main(void) {
  LOG_INF("K Watch starting");

  buttons_init();
  rtc_init();
  led_init();
  power_init();

  led_test();

  if (ble_init() < 0) {
    LOG_ERR("BLE init failed");
  }

  if (app_init() < 0) {
    LOG_ERR("UI init failed");
  }

  if (rtc_minute_alarm_enable() < 0) {
    LOG_ERR("RTC minute alarm init failed");
  }

  while (1) {
    app_task_handler();
  }
  return 0;
}
