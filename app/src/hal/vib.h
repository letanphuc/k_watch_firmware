/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef APP_HAL_VIB_H_
#define APP_HAL_VIB_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Start vibration with specified duty cycle and duration.
 *
 * @param duty Duty cycle (0-100)
 * @param duration_ms Duration in milliseconds
 */
void vib_start(uint8_t duty, uint32_t duration_ms);

#ifdef __cplusplus
}
#endif

#endif /* APP_HAL_VIB_H_ */
