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
#include "power.h"
#include "rtc.h"

LOG_MODULE_REGISTER(main);

int main(void) {
  LOG_INF("K Watch starting");

  buttons_init();
  rtc_init();
  power_init();

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
    uint32_t next_lvgl_timer = app_task_handler();
    if (next_lvgl_timer > 1000) {
      next_lvgl_timer = 1000;  // Cap to 1s to ensure timely updates
    }
    k_sem_take(&lvgl_refresh_sem, K_MSEC(next_lvgl_timer));
  }
  return 0;
}
