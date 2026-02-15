/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_drv2603

#include <app/drivers/vib.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(drv2603);

struct drv2603_config {
  struct gpio_dt_spec en_gpio;
  struct pwm_dt_spec pwm;
};

static int drv2603_set_intensity(const struct device* dev, uint8_t intensity) {
  const struct drv2603_config* config = dev->config;
  int ret;

  if (intensity == 0) {
    ret = gpio_pin_set_dt(&config->en_gpio, 0);
    if (ret < 0) {
      LOG_ERR("Could not disable EN GPIO (%d)", ret);
      return ret;
    }
    return pwm_set_pulse_dt(&config->pwm, 0);
  }

  ret = gpio_pin_set_dt(&config->en_gpio, 1);
  if (ret < 0) {
    LOG_ERR("Could not enable EN GPIO (%d)", ret);
    return ret;
  }

  /* PWM period is typically handled by pwm_set_pulse_dt which uses the period from DTS */
  uint32_t pulse = (uint32_t)((uint64_t)config->pwm.period * intensity / 100);
  return pwm_set_pulse_dt(&config->pwm, pulse);
}

static DEVICE_API(vib, drv2603_api) = {
    .set_intensity = drv2603_set_intensity,
};

static int drv2603_init(const struct device* dev) {
  const struct drv2603_config* config = dev->config;
  int ret;

  if (!gpio_is_ready_dt(&config->en_gpio)) {
    LOG_ERR("EN GPIO not ready");
    return -ENODEV;
  }

  if (!pwm_is_ready_dt(&config->pwm)) {
    LOG_ERR("PWM not ready");
    return -ENODEV;
  }

  ret = gpio_pin_configure_dt(&config->en_gpio, GPIO_OUTPUT_INACTIVE);
  if (ret < 0) {
    LOG_ERR("Could not configure EN GPIO (%d)", ret);
    return ret;
  }

  return 0;
}

#define DRV2603_DEFINE(inst)                                                                \
  static const struct drv2603_config drv2603_config##inst = {                               \
      .en_gpio = GPIO_DT_SPEC_INST_GET(inst, en_gpios),                                     \
      .pwm = PWM_DT_SPEC_INST_GET(inst),                                                    \
  };                                                                                        \
                                                                                            \
  DEVICE_DT_INST_DEFINE(inst, drv2603_init, NULL, NULL, &drv2603_config##inst, POST_KERNEL, \
                        CONFIG_HAPTICS_INIT_PRIORITY, &drv2603_api);

DT_INST_FOREACH_STATUS_OKAY(DRV2603_DEFINE)
