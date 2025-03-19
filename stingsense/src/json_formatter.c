#include "json_formatter.h"
#include <zephyr/data/json.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(json, CONFIG_LOG_DEFAULT_LEVEL);

static char json_buffer[512];

struct sensor_data {
    char datetime[32];
    double latitude;
    double longitude;
    double altitude;
    double x_accel;
    double y_accel;
    double z_accel;
};

static const struct json_obj_descr sensor_data_descr[] = {
    JSON_OBJ_DESCR_PRIM(struct sensor_data, datetime, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(struct sensor_data, latitude, JSON_TOK_FLOAT),
    JSON_OBJ_DESCR_PRIM(struct sensor_data, longitude, JSON_TOK_FLOAT),
    JSON_OBJ_DESCR_PRIM(struct sensor_data, altitude, JSON_TOK_FLOAT),
    JSON_OBJ_DESCR_PRIM(struct sensor_data, x_accel, JSON_TOK_FLOAT),
    JSON_OBJ_DESCR_PRIM(struct sensor_data, y_accel, JSON_TOK_FLOAT),
    JSON_OBJ_DESCR_PRIM(struct sensor_data, z_accel, JSON_TOK_FLOAT),
};

static int json_append_data(const char *bytes, size_t len, void *data)
{
    char *str = (char *)data;
    memcpy(str + strlen(str), bytes, len);
    return 0;
}

char *format_sensor_data_json(double x_accel, double y_accel, double z_accel,
                             struct gps_data *gps, struct datetime *dt)
{
    struct sensor_data data;
    
    // Format datetime
    snprintf(data.datetime, sizeof(data.datetime), 
             "%04d-%02d-%02d %02d:%02d:%02d EST",
             dt->year, dt->month, dt->day, dt->hour, dt->minute, dt->second);
    
    // Set GPS data
    data.latitude = gps->latitude;
    data.longitude = gps->longitude;
    data.altitude = gps->altitude;
    
    // Set accelerometer data
    data.x_accel = x_accel;
    data.y_accel = y_accel;
    data.z_accel = z_accel;
    
    // Clear buffer
    memset(json_buffer, 0, sizeof(json_buffer));
    
    // Encode data to JSON
    int ret = json_obj_encode(sensor_data_descr, 
                             ARRAY_SIZE(sensor_data_descr),
                             &data, json_append_data, json_buffer);
                            
    if (ret < 0) {
        LOG_ERR("Failed to encode data to JSON, error: %d", ret);
        strcpy(json_buffer, "{\"error\":\"Failed to encode JSON\"}");
    }
    
    return json_buffer;
}