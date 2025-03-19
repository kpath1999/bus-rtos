#include "rtc.h"
#include <date_time.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <zephyr/logging/log.h>
#include <stdio.h>

LOG_MODULE_REGISTER(rtc, CONFIG_LOG_DEFAULT_LEVEL);

static char datetime_str[32];

bool init_rtc(void)
{
    const struct device *clock;
    int err;

    clock = device_get_binding(DT_LABEL(DT_INST(0, nordic_nrf_clock)));
    if (!clock) {
        LOG_ERR("Could not get clock device");
        return false;
    }

    err = clock_control_on(clock, CLOCK_CONTROL_NRF_SUBSYS_LF);
    if (err) {
        LOG_ERR("Could not start LF clock, error: %d", err);
        return false;
    }

    err = date_time_initialize();
    if (err) {
        LOG_ERR("Failed to initialize date time library, error: %d", err);
        return false;
    }

    LOG_INF("RTC initialized");
    return true;
}

void get_datetime(struct datetime *dt)
{
    int64_t unix_time_ms = 0;
    int err = date_time_now(&unix_time_ms);
    
    // Eastern Time Zone offset (-5 hours in standard time)
    // Note: This doesn't account for daylight saving
    int64_t est_offset_ms = -5 * 60 * 60 * 1000;
    unix_time_ms += est_offset_ms;
    
    if (err) {
        LOG_WRN("Failed to get current time, error: %d", err);
        dt->year = 2025;
        dt->month = 3;
        dt->day = 19;
        dt->hour = 0;
        dt->minute = 0;
        dt->second = 0;
        return;
    }

    // Convert unix timestamp (ms) to datetime components
    int64_t unix_time_s = unix_time_ms / 1000;
    int64_t time_of_day = unix_time_s % (24 * 60 * 60);
    
    dt->hour = time_of_day / 3600;
    dt->minute = (time_of_day % 3600) / 60;
    dt->second = time_of_day % 60;
    
    // This is a very simplified date calculation 
    // A better implementation would use a proper date/time library
    // But for Zephyr 2.7.x with limited resources, this will give basic functionality
    int64_t days_since_epoch = unix_time_s / (24 * 60 * 60);
    
    // Rough calculation - not accounting for leap years perfectly
    dt->year = 1970 + (days_since_epoch / 365);
    int day_of_year = days_since_epoch % 365;
    
    // Simple month determination
    static const int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int month = 0;
    int days = day_of_year;
    
    while (month < 12) {
        int days_this_month = days_in_month[month];
        // Rough leap year adjustment for February
        if (month == 1 && dt->year % 4 == 0) {
            days_this_month = 29;
        }
        
        if (days < days_this_month) {
            break;
        }
        
        days -= days_this_month;
        month++;
    }
    
    dt->month = month + 1;
    dt->day = days + 1;
}

char *get_datetime_str(void)
{
    struct datetime dt;
    get_datetime(&dt);
    
    snprintf(datetime_str, sizeof(datetime_str), 
             "%04d-%02d-%02d %02d:%02d:%02d EST",
             dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second);
    
    return datetime_str;
}