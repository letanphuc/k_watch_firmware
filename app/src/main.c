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
#include "hal/vib.h"
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

  vib_start(100, 2000);

  if (rtc_minute_alarm_enable() < 0) {
    LOG_ERR("RTC minute alarm init failed");
  }

  while (1) {
    app_task_handler();
  }
  return 0;
}
