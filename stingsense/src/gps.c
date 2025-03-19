#include "gps.h"
#include <zephyr/kernel.h>
#include <modem/at_cmd.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include <string.h>

LOG_MODULE_REGISTER(gps, CONFIG_LOG_DEFAULT_LEVEL);

#define GPS_AT_CMD_BUF_SIZE 256
static char at_resp_buf[GPS_AT_CMD_BUF_SIZE];

bool init_gps(void)
{
    int err;

    // Set modem to LTE-M + GPS mode
    err = at_cmd_write("AT%XSYSTEMMODE=1,0,1,0", NULL, 0, NULL);
    if (err) {
        LOG_ERR("Failed to set LTE-M + GPS mode, error: %d", err);
        return false;
    }

    // Configure GPS antenna coexistence settings (recommended)
    err = at_cmd_write("AT%XCOEX0=1,1,1565,1586", NULL, 0, NULL);
    if (err) {
        LOG_ERR("Failed to configure COEX0 for GPS: %d", err);
        return false;
    }

    LOG_INF("GPS initialized via AT commands");
    return true;
}

static bool parse_gps_response(const char *resp, struct gps_data *data)
{
    int fix_status;
    int year, month, day, hour, minute;
    float seconds;
    float latitude, longitude, altitude, speed;

    // Parse the response from AT command +CGPSINFO
    // Response format: +CGPSINFO: <lat>,<N/S>,<lon>,<E/W>,<date>,<UTC time>,<altitude>,<speed>
    if (strstr(resp, "+CGPSINFO: ,,,,,,,") != NULL) {
        LOG_WRN("No GPS fix available yet");
        return false; // No fix yet
    }

    char lat_dir, lon_dir;
    char date[7], utc_time[10];

    int matched = sscanf(resp,
        "+CGPSINFO: %f,%c,%f,%c,%6[^,],%9[^,],%f,%f",
        &latitude,
        &lat_dir,
        &longitude,
        &lon_dir,
        date,
        utc_time,
        &altitude,
        &speed);

    if (matched != 8) {
        LOG_ERR("Failed to parse GPS response correctly");
        return false;
    }

    // Convert latitude and longitude to decimal degrees
    int lat_deg = (int)(latitude / 100);
    float lat_min = latitude - (lat_deg * 100);
    data->latitude = lat_deg + lat_min / 60.0;
    if (lat_dir == 'S') data->latitude *= -1;

    int lon_deg = (int)(longitude / 100);
    float lon_min = longitude - (lon_deg * 100);
    data->longitude = lon_deg + lon_min / 60.0;
    if (lon_dir == 'W') data->longitude *= -1;

    data->altitude = altitude;
    data->speed = speed;

    // Parse date and time into readable format
    sscanf(date, "%2d%2d%2d", &day, &month, &year);
    sscanf(utc_time, "%2d%2d%f", &hour, &minute, &seconds);

    snprintf(data->datetime, sizeof(data->datetime),
             "20%02d-%02d-%02d %02d:%02d:%02.0f UTC",
             year, month, day,
             hour, minute, seconds);

    return true;
}

bool get_gps_data(struct gps_data *data)
{
    int err;

    // Start GPS session
    err = at_cmd_write("AT+CFUN=31", NULL, 0, NULL); // Activate GNSS only mode temporarily
    if (err) {
        LOG_ERR("Failed to activate GPS mode: %d", err);
        return false;
    }

    LOG_INF("Waiting for GPS fix...");
    
    bool fix_acquired = false;
    
    for (int retries = 0; retries < 120; retries++) { // wait up to ~120 seconds
        k_sleep(K_SECONDS(1));

        memset(at_resp_buf, 0, sizeof(at_resp_buf));
        
        err = at_cmd_write("AT+CGPSINFO", at_resp_buf, sizeof(at_resp_buf), NULL);
        
        if (err) {
            LOG_ERR("Failed to query GPS info: %d", err);
            continue; // retry again
        }

        if (parse_gps_response(at_resp_buf, data)) {
            fix_acquired = true;
            break; // Fix acquired successfully
        }
        
        LOG_INF("GPS fix not yet available (%d/120)", retries + 1);
    }

    // Restore normal LTE functionality after GPS session
    err = at_cmd_write("AT+CFUN=1", NULL, 0, NULL); 
    if (err) {
        LOG_WRN("Failed to restore LTE functionality after GPS session");
        // Not critical; proceed anyway
    }

    if (!fix_acquired) {
        LOG_ERR("GPS fix was not acquired within timeout");
        return false;
    }

    LOG_INF("GPS fix acquired successfully");
    
    return true;
}
