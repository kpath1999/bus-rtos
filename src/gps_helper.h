#ifndef GPS_HELPER_H
#define GPS_HELPER_H

#include <stdbool.h>

struct gps_data {
    double latitude;
    double longitude;
    double altitude;
    bool valid_fix;
};

bool gps_init(void);
bool gps_fetch_data(struct gps_data *data);

#endif // GPS_HELPER_H
