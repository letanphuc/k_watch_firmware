#ifndef POWER_H
#define POWER_H

#include <stdint.h>
#include <zephyr/drivers/sensor.h>

struct power_data {
  struct sensor_value vbat;
  struct sensor_value ibat;
  struct sensor_value ntc_temp;
  struct sensor_value die_temp;
  uint8_t battery_percent;
  struct sensor_value charger_status;
  struct sensor_value charger_error;
  struct sensor_value vbus_status;
};

int power_init(void);
int power_read(struct power_data* data);
void power_test(void);

#endif /* POWER_H */
