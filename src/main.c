/*
 * Copyright (c) 2019-2022 Actinius
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "accelerometer.h"
#include "rtc.h"
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

// Function to format time in 12-hour format with AM/PM
void format_time_12h(int hour, int minute, int second, char *buffer, size_t size)
{
    const char *period = (hour >= 12) ? "PM" : "AM";
    int display_hour = (hour > 12) ? hour - 12 : hour;
    
    // Handle midnight (0 hour) as 12 AM
    if (hour == 0) {
        display_hour = 12;
    }
    
    snprintf(buffer, size, "%d:%02d:%02d %s", display_hour, minute, second, period);
}

int main(void)
{
    LOG_INF("Starting Sensor Data Application");
    
    // Initialize accelerometer
    if (!init_accelerometer()) {
        LOG_ERR("Failed to initialize accelerometer");
        return -1;
    }
    
    // Initialize RTC
    if (!init_rtc()) {
        LOG_ERR("Failed to initialize RTC");
        return -1;
    }
    
    // Set initial time (March 19, 2025, 8:25 PM)
    set_rtc_time(2025, 3, 19, 20, 25, 0);
    
    char time_str[20];
    
    while (1) {
        // Clear screen
        printk("\033[1;1H");
        printk("\033[2J");
        
        // Get accelerometer data
        double x_accel = 0;
        double y_accel = 0;
        double z_accel = 0;
        get_accelerometer_data(&x_accel, &y_accel, &z_accel);
        
        // Get current time
        struct datetime dt;
        get_datetime(&dt);
        
        // Format time in 12-hour format
        format_time_12h(dt.hour, dt.minute, dt.second, time_str, sizeof(time_str));
        
        // Print combined sensor data to serial console
        printk("Sensor Data Dashboard\n");
        printk("===============================================================================\n");
        
        // Date and Time section
        printk("Date and Time:\n");
        printf("  Date: %04d-%02d-%02d\n", dt.year, dt.month, dt.day);
        printf("  Time: %s\n", time_str);
        printk("\n");
        
        // Accelerometer section
        printk("Acceleration Values:\n");
        printf("  X: %lf (m/s^2)\n", x_accel);
        printf("  Y: %lf (m/s^2)\n", y_accel);
        printf("  Z: %lf (m/s^2)\n", z_accel);
        
        printk("===============================================================================\n");
        printk("Last updated: %04d-%02d-%02d %02d:%02d:%02d\n", 
               dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second);
        
        // Wait before updating (500ms for more responsive updates)
        k_sleep(K_MSEC(500));
    }
    
    return 0;
}
