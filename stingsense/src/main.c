/*
 * Copyright (c) 2019-2022 Actinius
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "accelerometer.h"
#include "gps.h"
#include "rtc.h"
#include "lte.h"
#include "json_formatter.h"

#include <stdio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

void main(void)
{
    LOG_INF("Starting sensor data collection application");
    
    // Initialize all subsystems
    if (!init_accelerometer()) {
        LOG_ERR("Failed to initialize accelerometer");
        return;
    }
    
    struct gps_data gps_info;
    if (!init_gps()) {
        LOG_WRN("Failed to initialize GPS, continuing without GPS data");
    } else {
        printk("GPS initialized successfully\n");
    }
    
    if (!init_rtc()) {
        LOG_WRN("Failed to initialize RTC, continuing with default time");
    }
    
    if (!init_lte()) {
        LOG_WRN("Failed to initialize LTE, data will not be transmitted");
    }
    
    LOG_INF("All subsystems initialized, starting main loop");
    
    while (1) {
        // Clear screen for serial output
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
        
        // Format data as JSON
        char *json_data = format_sensor_data_json(x_accel, y_accel, z_accel, &gps_data, &dt);

        printk("Attempting to get GPS data...\n");

        if (get_gps_data(&gps_info)) {
            printk("GPS Data:\n");
            printk("Latitude: %.6f\n", gps_info.latitude);
            printk("Longitude: %.6f\n", gps_info.longitude);
            printk("Altitude: %.2f m\n", gps_info.altitude);
            printk("Speed: %.2f knots\n", gps_info.speed);
            printk("Timestamp: %s\n", gps_info.datetime);
        } else {
            printk("Failed to acquire GPS fix.\n");
        }

        k_sleep(K_SECONDS(180)); // Wait before next attempt
        
        // Print data to serial console
        printk("Sensor Data\n");
        printk("-------------------------------------------------------------------------------\n");
        printf("Date/Time: %04d-%02d-%02d %02d:%02d:%02d EST\n", 
               dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second);
        printf("Acceleration: X: %lf (m/s^2), Y: %lf (m/s^2), Z: %lf (m/s^2)\n", 
               x_accel, y_accel, z_accel);
        printk("-------------------------------------------------------------------------------\n");
        printf("JSON Data: %s\n", json_data);
        printk("-------------------------------------------------------------------------------\n");
        
        // Send data over LTE if connected
        if (lte_is_connected()) {
            if (lte_send_data(json_data, strlen(json_data))) {
                printk("Data successfully sent over LTE\n");
            } else {
                printk("Failed to send data over LTE\n");
            }
        } else {
            printk("LTE not connected, data not sent\n");
			printk("LTE connection lost, attempting to reconnect...\n");
    		if (!lte_reconnect()) {
        		printk("Failed to reconnect LTE\n");
        		// Handle reconnection failure
				lte_shutdown();
    		} else {
        		printk("LTE reconnected successfully\n");
    		}
        }

        
        // Wait for 3 seconds before collecting new data
        k_sleep(K_SECONDS(3));
    }
}


