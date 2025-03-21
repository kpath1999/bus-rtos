#include "rtc.h"
#include <zephyr/device.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/logging/log.h>
#include <stdio.h>

LOG_MODULE_REGISTER(rtc_module, CONFIG_LOG_DEFAULT_LEVEL);

static const struct device *rtc_dev;
static char datetime_str[32];

bool init_rtc(void)
{
    // Get RTC device
    rtc_dev = DEVICE_DT_GET(DT_ALIAS(rtc));
    if (!device_is_ready(rtc_dev)) {
        LOG_ERR("RTC device not ready");
        return false;
    }

    LOG_INF("RTC device initialized");
    return true;
}

int set_rtc_time(int year, int month, int day, int hour, int minute, int second)
{
    struct rtc_time time = {
        .tm_year = year - 1900,  // Years since 1900
        .tm_mon = month - 1,     // Months since January [0-11]
        .tm_mday = day,          // Day of the month [1-31]
        .tm_hour = hour,         // Hours since midnight [0-23]
        .tm_min = minute,        // Minutes after the hour [0-59]
        .tm_sec = second,        // Seconds after the minute [0-59]
        .tm_wday = -1,           // Let the RTC calculate the weekday
        .tm_yday = -1,           // Let the RTC calculate the yearday
        .tm_isdst = -1,          // Let the RTC determine DST
        .tm_nsec = 0             // Nanoseconds
    };

    int ret = rtc_set_time(rtc_dev, &time);
    if (ret) {
        LOG_ERR("Failed to set RTC time: %d", ret);
        return ret;
    }

    LOG_INF("RTC time set to %04d-%02d-%02d %02d:%02d:%02d",
            year, month, day, hour, minute, second);
    return 0;
}

void get_datetime(struct datetime *dt)
{
    struct rtc_time time;
    int ret = rtc_get_time(rtc_dev, &time);
    
    if (ret) {
        LOG_WRN("Failed to get RTC time: %d", ret);
        // Provide default values if RTC read fails
        dt->year = 2025;
        dt->month = 3;
        dt->day = 19;
        dt->hour = 20;
        dt->minute = 25;
        dt->second = 0;
        return;
    }
    
    // Convert from RTC time format to our datetime structure
    dt->year = time.tm_year + 1900;  // Convert back to calendar year
    dt->month = time.tm_mon + 1;     // Convert back to calendar month [1-12]
    dt->day = time.tm_mday;
    dt->hour = time.tm_hour;
    dt->minute = time.tm_min;
    dt->second = time.tm_sec;
}

char *get_datetime_str(void)
{
    struct datetime dt;
    get_datetime(&dt);
    
    snprintf(datetime_str, sizeof(datetime_str), 
             "%04d-%02d-%02d %02d:%02d:%02d",
             dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second);
    
    return datetime_str;
}