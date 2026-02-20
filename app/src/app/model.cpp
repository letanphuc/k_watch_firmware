#include "model.hpp"

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(model_cpp, LOG_LEVEL_INF);

void Model::add_notification(const ancs_noti_info_t* noti) {
  if (noti == nullptr) {
    return;
  }

  LOG_INF("Adding notification: %s - %s - %s", noti->title, noti->message, noti->app);

  // If we are overwriting an existing notification, free its strings
  if (noti_count_ == MAX_NOTIFICATIONS) {
    if (notifications_[head_].title) k_free(notifications_[head_].title);
    if (notifications_[head_].message) k_free(notifications_[head_].message);
    if (notifications_[head_].app) k_free(notifications_[head_].app);
  }

  // Copy notification (shallow copy of pointers to transfer ownership)
  notifications_[head_] = *noti;

  // Update head (circularly)
  head_ = (head_ + 1) % MAX_NOTIFICATIONS;
  LOG_INF("Updated head to: %d", head_);

  // Update count (up to MAX)
  if (noti_count_ < MAX_NOTIFICATIONS) {
    noti_count_++;
  }
  LOG_INF("Updated noti_count to: %d", noti_count_);
}

const ancs_noti_info_t* Model::get_notification(uint8_t index) const {
  if (index >= noti_count_) {
    return nullptr;
  }

  // Browsing should probably be in reverse order (most recent first)
  // Most recent is at (head - 1 + MAX) % MAX
  // Second most recent at (head - 2 + MAX) % MAX
  int8_t target_idx = (int8_t)head_ - 1 - index;
  while (target_idx < 0) {
    target_idx += MAX_NOTIFICATIONS;
  }

  return &notifications_[target_idx];
}

void Model::dump_notifications() const {
  for (int i = 0; i < noti_count_; i++) {
    const ancs_noti_info_t* n = get_notification(i);
    LOG_INF("Notification %d: %s - %s - %s", i, n->title, n->message, n->app);
  }
}
