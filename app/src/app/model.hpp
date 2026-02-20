#ifndef APP_MODEL_HPP
#define APP_MODEL_HPP

#include <stdint.h>

#include <vector>

#include "../hal/ancs_client.hpp"

#define MAX_NOTIFICATIONS 10

class Model {
 public:
  static Model& instance() {
    static Model inst;
    return inst;
  }

  void add_notification(const ancs_noti_info_t* noti);
  uint8_t get_notification_count() const { return noti_count_; }
  const ancs_noti_info_t* get_notification(uint8_t index) const;
  void dump_notifications() const;

 private:
  Model() : noti_count_(0), head_(0) {}

  // Avoid dynamic allocation if possible for embedded, but using fixed array
  ancs_noti_info_t notifications_[MAX_NOTIFICATIONS];
  uint8_t noti_count_;
  uint8_t head_;
};

#endif  // APP_MODEL_HPP
