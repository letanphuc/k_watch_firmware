#ifndef RTC_HPP
#define RTC_HPP

#include <zephyr/drivers/rtc.h>

class Rtc {
 public:
  static Rtc& instance() {
    static Rtc inst;
    return inst;
  }

  int init();
  int time_get(struct rtc_time* time);
  int time_set(const struct rtc_time* time);
  int minute_alarm_enable();
  void test();

 private:
  Rtc() = default;
  int schedule_next_minute_alarm();
};

#endif /* RTC_HPP */
