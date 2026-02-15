#ifndef APP_H
#define APP_H

#include <stdint.h>
#include <zephyr/kernel.h>

typedef enum {
  APP_EVENT_BUTTON,
  APP_EVENT_RTC_ALARM,
  APP_EVENT_BLE_ANCS,
  APP_EVENT_BLE_CTS,
  APP_EVENT_BATTERY,
} app_event_type_t;

typedef struct {
  app_event_type_t type;
  union {
    uint32_t value;
    void* ptr;
  };
  size_t len;
} app_event_t;

int app_init(void);
uint32_t app_task_handler(void);
int app_event_post(app_event_t* event);

#endif /* APP_H */
