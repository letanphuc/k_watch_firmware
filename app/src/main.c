/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/poweroff.h>

#include "ble.h"
#include "buttons.h"
#include "power.h"
#include "rtc.h"
#include "ui.h"

LOG_MODULE_REGISTER(main);

int main(void) {
  LOG_INF("K Watch starting");

  buttons_init();
  rtc_init();
  power_init();

  if (ble_init() < 0) {
    LOG_ERR("BLE init failed");
  }

  if (ui_init() < 0) {
    LOG_ERR("UI init failed");
  }

  if (rtc_minute_alarm_enable() < 0) {
    LOG_ERR("RTC minute alarm init failed");
  }

  while (1) {
    uint32_t next_lvgl_timer = ui_task_handler();
    k_sem_take(&lvgl_refresh_sem, K_MSEC(next_lvgl_timer));
    ui_watchface_update();
  }
  return 0;
}
