#ifndef EVENT_H
#define EVENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <zephyr/kernel.h>

typedef enum {
  APP_EVENT_BUTTON,
  APP_EVENT_RTC_ALARM,
  APP_EVENT_BLE_ANCS,
  APP_EVENT_BLE_CTS,
  APP_EVENT_BATTERY,
  APP_EVENT_MODE_TIMEOUT,
} app_event_type_t;

typedef struct {
  app_event_type_t type;
  union {
    uint32_t value;
    void* ptr;
  };
  size_t len;
} app_event_t;

#ifdef __cplusplus
}
#endif

#endif /* EVENT_H */
