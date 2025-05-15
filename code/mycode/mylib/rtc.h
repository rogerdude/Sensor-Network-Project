#ifndef RTC_H
#define RTC_H

#include <zephyr/types.h>
#include <time.h>  // for struct tm, mktime, gmtime


void rtc_init(void);
void rtc_set_calendar_time(const struct tm *tm);
void rtc_get_calendar_time(struct tm *tm);
static int cmd_rtc_read(const struct shell *shell, size_t argc, char **argv);
static int cmd_rtc_write(const struct shell *shell, size_t argc, char **argv);
extern uint32_t rtc_get_time(void);
#endif /* RTC_H */
