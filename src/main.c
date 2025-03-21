#include "accelerometer.h"
#include "rtc.h"
#include "gps_helper.h"
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

// Function to format time in 12-hour format with AM/PM
void format_time_12h(int hour, int minute, int second, char *buffer, size_t size)
{
    const char *period = (hour >= 12) ? "PM" : "AM";
    int display_hour = (hour > 12) ? hour - 12 : hour;

    if (hour == 0) display_hour = 12;

    snprintf(buffer, size, "%d:%02d:%02d %s", display_hour, minute, second, period);
}

int main(void)
{
    LOG_INF("Starting Sensor Data Application");

    if (!init_accelerometer()) {
        LOG_ERR("Accelerometer init failed");
        return -1;
    }

    if (!init_rtc()) {
        LOG_ERR("RTC init failed");
        return -1;
    }

    if (!gps_init()) {
        LOG_ERR("GPS init failed");
        return -1;
    }

    set_rtc_time(2025, 3, 19, 20, 25, 0);

    char time_str[20];

    while (1) {
        printk("\033[1;1H\033[2J");

        double x_accel = 0, y_accel = 0, z_accel = 0;
        get_accelerometer_data(&x_accel, &y_accel, &z_accel);

        struct datetime dt;
        get_datetime(&dt);
        format_time_12h(dt.hour, dt.minute, dt.second, time_str, sizeof(time_str));

        struct gps_data gps_info;
        bool has_gps_fix = gps_fetch_data(&gps_info);

        printk("Sensor Data Dashboard\n");
        printk("===============================================\n");

        printf("Date: %04d-%02d-%02d\n", dt.year, dt.month, dt.day);
        printf("Time: %s EST\n\n", time_str);

        printf("Acceleration:\n X: %.2lf m/s^2\n Y: %.2lf m/s^2\n Z: %.2lf m/s^2\n\n",
            x_accel, y_accel, z_accel);

        if (has_gps_fix && gps_info.valid_fix) {
            printf("GPS Data:\n Latitude: %.6lf\n Longitude: %.6lf\n Altitude: %.2lf m\n",
                gps_info.latitude,
                gps_info.longitude,
                gps_info.altitude);
        } else {
            printf("GPS Data:\n No valid fix yet.\n");
        }

        printk("===============================================\n");
        
        k_sleep(K_SECONDS(3));
    }

    return 0;
}
