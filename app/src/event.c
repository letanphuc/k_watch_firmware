#include "event.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(event_module, LOG_LEVEL_DBG);

K_MSGQ_DEFINE(app_msgq, sizeof(app_event_t), 10, 4);

int event_post(app_event_t* event) {
  LOG_INF("Posting event type: %u", event->type);
  return k_msgq_put(&app_msgq, event, K_NO_WAIT);
}

int event_get(app_event_t* event, k_timeout_t timeout) { return k_msgq_get(&app_msgq, event, timeout); }
