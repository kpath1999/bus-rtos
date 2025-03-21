#include "gps_helper.h"
#include <modem/nrf_modem_lib.h>
#include <nrf_modem_at.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(gps_helper, CONFIG_LOG_DEFAULT_LEVEL);

bool gps_init(void)
{
    int err = nrf_modem_lib_init();
    if (err) {
        LOG_ERR("Modem library initialization failed: %d", err);
        return false;
    }

    // Configure modem for GNSS operation (LTE-M + GNSS mode)
    err = nrf_modem_at_printf("AT%%XSYSTEMMODE=1,0,1,0");
    if (err) {
        LOG_ERR("Failed to set system mode: %d", err);
        return false;
    }

    // Enable GNSS functionality
    err = nrf_modem_at_printf("AT+CFUN=31");
    if (err) {
        LOG_ERR("Failed to enable GNSS mode: %d", err);
        return false;
    }

    LOG_INF("GPS initialized successfully");
    return true;
}

bool gps_fetch_data(struct gps_data *data)
{
    char response[256];
    
    int err = nrf_modem_at_cmd(response, sizeof(response), "AT+CGPSINFO");
    if (err) {
        LOG_ERR("GPS data fetch failed: %d", err);
        data->valid_fix = false;
        return false;
    }

    // Check if fix is available
    if (strstr(response, "+CGPSINFO: ,,,,,,,") != NULL) {
        LOG_INF("No valid GPS fix yet.");
        data->valid_fix = false;
        return false;
    }

    // Parse NMEA response here to extract latitude, longitude, altitude...
    // For simplicity, let's assume parsing is done correctly:
    
    // Example parsed values (replace with actual parsing logic):
    data->latitude = 42.3601;
    data->longitude = -71.0589;
    data->altitude = 10.5;
    
    data->valid_fix = true;

    LOG_INF("GPS fix acquired.");
    
    return true;
}
