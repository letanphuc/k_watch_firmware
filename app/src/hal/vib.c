/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vib.h"

#include <app/drivers/vib.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(vib);

#define DRV2603_NODE DT_NODELABEL(drv2603)

static const struct device* vib_dev = DEVICE_DT_GET(DRV2603_NODE);

static struct k_timer vib_timer;

static void vib_stop_timer_handler(struct k_timer* timer) {
  if (!device_is_ready(vib_dev)) {
    return;
  }

  const struct vib_driver_api* api = vib_dev->api;
  api->set_intensity(vib_dev, 0);
}

void vib_start(uint8_t duty, uint32_t duration_ms) {
  if (!device_is_ready(vib_dev)) {
    LOG_ERR("Vibration device not ready");
    return;
  }

  static bool timer_init = false;
  if (!timer_init) {
    k_timer_init(&vib_timer, vib_stop_timer_handler, NULL);
    timer_init = true;
  }

  const struct vib_driver_api* api = vib_dev->api;
  api->set_intensity(vib_dev, duty);
  k_msleep(1000);
  api->set_intensity(vib_dev, 0);

  // if (duration_ms > 0) {
  //   k_timer_start(&vib_timer, K_MSEC(duration_ms), K_NO_WAIT);
  // }
}
