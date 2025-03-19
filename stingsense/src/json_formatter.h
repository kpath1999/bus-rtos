#ifndef _JSON_FORMATTER_H_
#define _JSON_FORMATTER_H_

#include <zephyr/kernel.h>
#include "accelerometer.h"
#include "gps.h"
#include "rtc.h"

char *format_sensor_data_json(double x_accel, double y_accel, double z_accel,
                              struct gps_data *gps, struct datetime *dt);

#endif // _JSON_FORMATTER_H_