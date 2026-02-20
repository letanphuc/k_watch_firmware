#include "rtc.h"

#include <zephyr/device.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/logging/log.h>

#include "event.h"

LOG_MODULE_REGISTER(rtc_module);

static const struct device* rtc_dev = DEVICE_DT_GET(DT_NODELABEL(rv8263));

static int rtc_schedule_next_minute_alarm(void) {
  struct rtc_time now;
  int ret = rtc_get_time(rtc_dev, &now);
  if (ret < 0) {
    LOG_ERR("Failed to get RTC time for alarm: %d", ret);
    return ret;
  }

  uint16_t supported_mask = 0;
  ret = rtc_alarm_get_supported_fields(rtc_dev, 0, &supported_mask);
  if (ret < 0) {
    LOG_ERR("Failed to get alarm fields: %d", ret);
    return ret;
  }

  struct rtc_time alarm_time = {0};
  int next_min = now.tm_min + 1;
  int next_hour = now.tm_hour;
  if (next_min >= 60) {
    next_min = 0;
    next_hour = (next_hour + 1) % 24;
  }

  uint16_t mask = 0;
  if (supported_mask & RTC_ALARM_TIME_MASK_SECOND) {
    alarm_time.tm_sec = 0;
    mask |= RTC_ALARM_TIME_MASK_SECOND;
  }
  if (supported_mask & RTC_ALARM_TIME_MASK_MINUTE) {
    alarm_time.tm_min = next_min;
    mask |= RTC_ALARM_TIME_MASK_MINUTE;
  }
  if (supported_mask & RTC_ALARM_TIME_MASK_HOUR) {
    alarm_time.tm_hour = next_hour;
    mask |= RTC_ALARM_TIME_MASK_HOUR;
  }

  if ((mask & RTC_ALARM_TIME_MASK_MINUTE) == 0) {
    LOG_ERR("Minute alarm not supported by RTC");
    return -ENOTSUP;
  }

  ret = rtc_alarm_set_time(rtc_dev, 0, mask, &alarm_time);
  if (ret < 0) {
    LOG_ERR("Failed to set minute alarm: %d", ret);
  }

  return ret;
}

static void rtc_minute_alarm_cb(const struct device* dev, uint16_t id, void* user_data) {
  (void)dev;
  (void)id;
  (void)user_data;

  LOG_INF("RTC minute alarm");
  app_event_t event = {
      .type = APP_EVENT_RTC_ALARM,
      .len = 0,
  };
  event_post(&event);
  rtc_schedule_next_minute_alarm();
}

int rtc_init(void) {
  if (!device_is_ready(rtc_dev)) {
    LOG_ERR("RTC device not ready");
    return -ENODEV;
  }

  LOG_INF("RTC initialized");
  return 0;
}

int rtc_time_get(struct rtc_time* time) {
  if (!device_is_ready(rtc_dev)) {
    return -ENODEV;
  }

  int ret = rtc_get_time(rtc_dev, time);
  if (ret < 0) {
    LOG_ERR("Failed to get RTC time: %d", ret);
    return ret;
  }

  return 0;
}

int rtc_time_set(const struct rtc_time* time) {
  LOG_INF("Setting RTC time: %04d-%02d-%02d %02d:%02d:%02d", time->tm_year + 1900, time->tm_mon + 1, time->tm_mday,
          time->tm_hour, time->tm_min, time->tm_sec);
  if (!device_is_ready(rtc_dev)) {
    return -ENODEV;
  }

  int ret = rtc_set_time(rtc_dev, time);
  if (ret < 0) {
    LOG_ERR("Failed to set RTC time: %d", ret);
    return ret;
  }

  LOG_INF("RTC time set");
  return 0;
}

int rtc_minute_alarm_enable(void) {
  if (!device_is_ready(rtc_dev)) {
    return -ENODEV;
  }

  int ret = rtc_alarm_set_callback(rtc_dev, 0, rtc_minute_alarm_cb, NULL);
  if (ret < 0) {
    LOG_ERR("Failed to set alarm callback: %d", ret);
    return ret;
  }

  return rtc_schedule_next_minute_alarm();
}

void rtc_test(void) {
  if (rtc_init() < 0) {
    return;
  }

  /* Set time: 2026-02-12 10:30:00 */
  struct rtc_time set_time = {
      .tm_sec = 0,
      .tm_min = 30,
      .tm_hour = 10,
      .tm_mday = 12,
      .tm_mon = 1,    /* February (0-based) */
      .tm_year = 126, /* Years since 1900 */
      .tm_wday = 4,   /* Thursday */
  };

  LOG_INF("Setting RTC time: %04d-%02d-%02d %02d:%02d:%02d", set_time.tm_year + 1900, set_time.tm_mon + 1,
          set_time.tm_mday, set_time.tm_hour, set_time.tm_min, set_time.tm_sec);

  if (rtc_time_set(&set_time) < 0) {
    return;
  }

  /* Read back time */
  struct rtc_time get_time;
  if (rtc_time_get(&get_time) < 0) {
    return;
  }

  LOG_INF("RTC time read: %04d-%02d-%02d %02d:%02d:%02d", get_time.tm_year + 1900, get_time.tm_mon + 1,
          get_time.tm_mday, get_time.tm_hour, get_time.tm_min, get_time.tm_sec);

  k_sleep(K_SECONDS(2));
  /* Read back time */
  if (rtc_time_get(&get_time) < 0) {
    return;
  }

  LOG_INF("RTC time read: %04d-%02d-%02d %02d:%02d:%02d", get_time.tm_year + 1900, get_time.tm_mon + 1,
          get_time.tm_mday, get_time.tm_hour, get_time.tm_min, get_time.tm_sec);
}
