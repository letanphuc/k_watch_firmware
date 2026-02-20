#ifndef POWER_HPP
#define POWER_HPP

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

class Power {
 public:
  static Power& instance() {
    static Power inst;
    return inst;
  }

  int init();
  int read(struct power_data* data);
  void test();

 private:
  Power() = default;
};

#endif /* POWER_HPP */
