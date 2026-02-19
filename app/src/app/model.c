#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "model.h"

LOG_MODULE_REGISTER(model);

static ancs_noti_info_t notifications[MAX_NOTIFICATIONS];
static uint8_t noti_count = 0;
static uint8_t head = 0;  // Index for the next new notification

void model_add_notification(const ancs_noti_info_t* noti) {
  if (noti == NULL) {
    return;
  }

  LOG_INF("Adding notification: %s - %s - %s", noti->title, noti->message, noti->app);

  // If we are overwriting an existing notification, free its strings
  if (noti_count == MAX_NOTIFICATIONS) {
    if (notifications[head].title) k_free(notifications[head].title);
    if (notifications[head].message) k_free(notifications[head].message);
    if (notifications[head].app) k_free(notifications[head].app);
  }

  // Copy notification (shallow copy of pointers to transfer ownership)
  notifications[head] = *noti;

  // Update head (circularly)
  head = (head + 1) % MAX_NOTIFICATIONS;
  LOG_INF("Updated head to: %d", head);

  // Update count (up to MAX)
  if (noti_count < MAX_NOTIFICATIONS) {
    noti_count++;
  }
  LOG_INF("Updated noti_count to: %d", noti_count);
}

uint8_t model_get_notification_count(void) { return noti_count; }

const ancs_noti_info_t* model_get_notification(uint8_t index) {
  if (index >= noti_count) {
    return NULL;
  }

  // Browsing should probably be in reverse order (most recent first)
  // Most recent is at (head - 1 + MAX) % MAX
  // Second most recent at (head - 2 + MAX) % MAX
  int8_t target_idx = (int8_t)head - 1 - index;
  while (target_idx < 0) {
    target_idx += MAX_NOTIFICATIONS;
  }

  return &notifications[target_idx];
}

void model_dump_notifications(void) {
  for (int i = 0; i < noti_count; i++) {
    const ancs_noti_info_t* n = model_get_notification(i);
    LOG_INF("Notification %d: %s - %s - %s", i, n->title, n->message, n->app);
  }
}
