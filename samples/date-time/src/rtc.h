#ifndef _RTC_H_
#define _RTC_H_

#include <zephyr/kernel.h>
#include <stdbool.h>

struct datetime {
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
};

bool init_rtc(void);
int set_rtc_time(int year, int month, int day, int hour, int minute, int second);
void get_datetime(struct datetime *dt);
char *get_datetime_str(void);

#endif // _RTC_H_