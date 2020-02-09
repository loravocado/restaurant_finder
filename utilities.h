#pragma once

#include <Arduino.h>

// These constants are for the 2048 by 2048 map .
# define MAP_WIDTH 2048
# define MAP_HEIGHT 2048
# define LAT_NORTH 53618581
# define LAT_SOUTH 53409531
# define LON_WEST -113686521
# define LON_EAST -113334961

struct restaurant {
  int32_t lat;
  int32_t lon;
  uint8_t rating; // from 0 to 10
  char name[55];
};

struct RestDist {
    int32_t index; // [0, NUM_RESTAURANTS)
    int32_t dist; 
};

// These functions convert between x/y map position and lat /lon
// (and vice versa .)
int16_t lon_to_x (int32_t lon) {
    return map ( lon , LON_WEST , LON_EAST , 0 , MAP_WIDTH );
}

int16_t lat_to_y (int32_t lat) {
    return map ( lat , LAT_NORTH , LAT_SOUTH , 0 , MAP_HEIGHT );
}

int32_t x_to_lon (int16_t x) {
    return map (x , 0 , MAP_WIDTH , LON_WEST , LON_EAST );
}

int32_t y_to_lat (int16_t y) {
    return map (y , 0 , MAP_HEIGHT , LAT_NORTH , LAT_SOUTH );
}