#ifndef _GPS_H_
#define _GPS_H_

#include <zephyr/kernel.h>
#include <stdbool.h>

struct gps_data {
    double latitude;
    double longitude;
    double altitude;
    double speed;
    char datetime[32]; // formatted date-time string
};

bool init_gps(void);
bool get_gps_data(struct gps_data *data);

#endif // _GPS_H_
