#ifndef RTC_H
#define RTC_H

#include <zephyr/drivers/rtc.h>

int rtc_init(void);
int rtc_time_get(struct rtc_time* time);
int rtc_time_set(const struct rtc_time* time);
int rtc_minute_alarm_enable(void);
void rtc_test(void);

#endif /* RTC_H */
