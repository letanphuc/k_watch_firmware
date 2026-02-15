/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef APP_DRIVERS_VIB_H_
#define APP_DRIVERS_VIB_H_

#include <stdint.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*vib_set_intensity_t)(const struct device *dev, uint8_t intensity);

struct vib_driver_api {
  vib_set_intensity_t set_intensity;
};

#ifdef __cplusplus
}
#endif

#endif /* APP_DRIVERS_VIB_H_ */
