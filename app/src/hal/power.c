#include "power.h"

#include <errno.h>
#include <nrf_fuel_gauge.h>
#include <stdbool.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/npm13xx_charger.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "event.h"

LOG_MODULE_REGISTER(power);

#define PMIC_CHARGER_NODE DT_NODELABEL(pmic_charger)

#if !DT_NODE_HAS_STATUS(PMIC_CHARGER_NODE, okay)
#error "pmic_charger node is not okay in devicetree"
#endif

static const struct device* const charger_dev = DEVICE_DT_GET(PMIC_CHARGER_NODE);
static int64_t fuel_gauge_ref_time;
static bool fuel_gauge_initialized;
uint32_t battery_percent = 0;

static const struct battery_model battery_model = {
#include "battery_model.inc"
};

static int frac_abs(int value) { return (value < 0) ? -value : value; }

static float sensor_value_to_float_local(const struct sensor_value* value) {
  return (float)value->val1 + ((float)value->val2 / 1000000.0f);
}

static int fuel_gauge_init_from_sensor(const struct power_data* data) {
  struct nrf_fuel_gauge_init_parameters params = {
      .model = &battery_model,
      .opt_params = NULL,
      .state = NULL,
  };

  params.v0 = sensor_value_to_float_local(&data->vbat);
  params.i0 = -sensor_value_to_float_local(&data->ibat);
  params.t0 = sensor_value_to_float_local(&data->ntc_temp);

  int ret = nrf_fuel_gauge_init(&params, NULL);
  if (ret < 0) {
    LOG_ERR("Fuel gauge init failed: %d", ret);
    return ret;
  }

  fuel_gauge_ref_time = k_uptime_get();
  fuel_gauge_initialized = true;
  return 0;
}

static int fuel_gauge_update_percent(struct power_data* data) {
  float voltage;
  float current;
  float temp;
  float delta_s;
  float soc;
  float tte;
  float ttf;
  int64_t ibat_ua;
  int64_t discharge_ua;

  voltage = sensor_value_to_float_local(&data->vbat);
  current = -sensor_value_to_float_local(&data->ibat);
  int64_t ibat_frac_ma;
  temp = sensor_value_to_float_local(&data->ntc_temp);

  delta_s = (float)k_uptime_delta(&fuel_gauge_ref_time) / 1000.0f;
  if (delta_s < 0.0f) {
    delta_s = 0.0f;
  }

  soc = nrf_fuel_gauge_process(voltage, current, temp, delta_s, NULL);
  tte = nrf_fuel_gauge_tte_get();
  ttf = nrf_fuel_gauge_ttf_get();

  ibat_ua = ((int64_t)data->ibat.val1 * 1000000LL) + data->ibat.val2;
  discharge_ua = (ibat_ua < 0) ? -ibat_ua : 0;

  ibat_frac_ma = ibat_ua % 1000LL;
  if (ibat_frac_ma < 0) {
    ibat_frac_ma = -ibat_frac_ma;
  }
  LOG_INF("IBAT(signed): %lld.%03lld mA", ibat_ua / 1000LL, ibat_frac_ma);
  LOG_INF("Discharge current: %lld.%03lld mA", discharge_ua / 1000LL, discharge_ua % 1000LL);
  LOG_INF("SoC: %d.%02d, TTE: %d, TTF: %d", (int)soc, frac_abs((int)((soc - (int)soc) * 100.0f)), (int)tte, (int)ttf);
  if (soc < 0.0f) {
    soc = 0.0f;
  } else if (soc > 100.0f) {
    soc = 100.0f;
  }

  data->battery_percent = (uint8_t)(soc + 0.5f);

  return 0;
}

static struct k_work_delayable power_report_work;

static void power_report_handler(struct k_work* work) {
  struct power_data data;
  if (power_read(&data) == 0) {
    LOG_INF("SOC=%u%% IBAT=%d.%03lldmA VBAT=%d.%03dV VBUS=0x%02x CHG=0x%02x", data.battery_percent,
            (int)(sensor_value_to_float_local(&data.ibat) * 1000.0f), (long long)frac_abs(data.ibat.val2 / 1000),
            data.vbat.val1, frac_abs(data.vbat.val2 / 1000), data.vbus_status.val1, data.charger_status.val1);
    battery_percent = data.battery_percent;

    app_event_t event = {
        .type = APP_EVENT_BATTERY,
        .value = data.battery_percent,
        .len = 0,
    };
    event_post(&event);
  }
  k_work_reschedule(&power_report_work, K_MINUTES(1));
}

int power_init(void) {
  if (!device_is_ready(charger_dev)) {
    LOG_ERR("PMIC charger device not ready");
    return -ENODEV;
  }

  LOG_INF("PMIC charger ready: %s", charger_dev->name);
  fuel_gauge_initialized = false;

  k_work_init_delayable(&power_report_work, power_report_handler);
  k_work_reschedule(&power_report_work, K_NO_WAIT);

  return 0;
}

int power_read(struct power_data* data) {
  int ret;

  if (data == NULL) {
    return -EINVAL;
  }

  if (!device_is_ready(charger_dev)) {
    return -ENODEV;
  }

  ret = sensor_sample_fetch(charger_dev);
  if (ret < 0) {
    LOG_ERR("sensor_sample_fetch failed: %d", ret);
    return ret;
  }

  ret = sensor_channel_get(charger_dev, SENSOR_CHAN_GAUGE_VOLTAGE, &data->vbat);
  if (ret < 0) {
    LOG_ERR("Failed to read battery voltage: %d", ret);
    return ret;
  }

  ret = sensor_channel_get(charger_dev, SENSOR_CHAN_GAUGE_AVG_CURRENT, &data->ibat);
  if (ret < 0) {
    LOG_ERR("Failed to read battery current: %d", ret);
    return ret;
  }

  ret = sensor_channel_get(charger_dev, SENSOR_CHAN_GAUGE_TEMP, &data->ntc_temp);
  if (ret < 0) {
    LOG_ERR("Failed to read battery temp: %d", ret);
    return ret;
  }

  ret = sensor_channel_get(charger_dev, SENSOR_CHAN_DIE_TEMP, &data->die_temp);
  if (ret < 0) {
    LOG_ERR("Failed to read die temp: %d", ret);
    return ret;
  }

  if (!fuel_gauge_initialized) {
    ret = fuel_gauge_init_from_sensor(data);
    if (ret < 0) {
      return ret;
    }
  }

  ret = fuel_gauge_update_percent(data);
  if (ret < 0) {
    return ret;
  }

  ret = sensor_channel_get(charger_dev, SENSOR_CHAN_NPM13XX_CHARGER_STATUS, &data->charger_status);
  if (ret < 0) {
    LOG_ERR("Failed to read charger status: %d", ret);
    return ret;
  }

  ret = sensor_channel_get(charger_dev, SENSOR_CHAN_NPM13XX_CHARGER_ERROR, &data->charger_error);
  if (ret < 0) {
    LOG_ERR("Failed to read charger error: %d", ret);
    return ret;
  }

  ret = sensor_channel_get(charger_dev, SENSOR_CHAN_NPM13XX_CHARGER_VBUS_STATUS, &data->vbus_status);
  if (ret < 0) {
    LOG_ERR("Failed to read VBUS status: %d", ret);
    return ret;
  }

  return 0;
}
