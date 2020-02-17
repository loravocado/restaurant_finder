#pragma once

#include <Arduino.h>

#include "config.h"

// These constants are for the 2048 by 2048 map .
#define MAP_WIDTH 2048
#define MAP_HEIGHT 2048
#define LAT_NORTH 5361858l
#define LAT_SOUTH 5340953l
#define LON_WEST -11368652l
#define LON_EAST -11333496l

enum sortState { QSORT, ISORT, BOTH };

struct restaurant {
  int32_t lat;
  int32_t lon;
  uint8_t rating; // from 0 to 10
  char name[55];
};

struct RestDist {
  uint16_t index; // [0, NUM_RESTAURANTS)
  uint16_t dist; 
};

struct Coord {
  int32_t X;
  int32_t Y;
};

/**
 * Swap function for two restaurants. Swaps the two input restaurants.
 *
 * @param a Restaurant 1.
 * @param b Restaurant 2.
 */
void swapRestaurants(RestDist* a, RestDist* b) {
  RestDist temp = *a;
  *a = *b;
  *b = temp;
}

/**
 * Insertion sort. Sorts restaurants based on Manhattan distance.
 *
 * @param n Length of the array.
 * @param A The array to sort.
 */
void isort(int n, RestDist A[]) {
  int i = 1;

  while (i < n) {
    int j = i;

    while (j > 0 && A[j - 1].dist > A[j].dist) {
      swapRestaurants(&A[j - 1], &A[j]);
      j--;
    }

    i++;
  }
}

/**
 * Partition function.
 * 
 * With last element as pivot, we place other smaller elements to the left of 
 * the piviot and larger elements to the right of the pivot. 
 *
 * @param A The array to partition.
 * @param low The starting index.
 * @param high The ending index.
 * 
 * @returns 
 */
int32_t partition (RestDist A[], int low, int high) {
  int32_t pivot = A[high].dist;
  int32_t j = (low - 1);

  for (int i = low; i <= high - 1; i++) {
    if (A[i].dist < pivot) {
      j++;
      swapRestaurants(&A[i], &A[j]);
    }
  }

  swapRestaurants(&A[j + 1], &A[high]);
  return (j + 1);
}

/**
 * Quick sort. Sorts restaurants based on Manhattan distance.
 *
 * @param A The array to sort.
 * @param low The starting index.
 * @param high The ending index.
 */
void qsort(RestDist A[], int low = 0, int high = NUM_RESTAURANTS - 1) {
  if (low < high) {
    int32_t part = partition(A, low, high);

    qsort(A, low, part - 1);
    qsort(A, part + 1, high);
  }
}

/**
 * The following functions were provided from eClass. They convert between
 * x/y map positions and latitude/longitude.
 */
int16_t lon_to_x(int32_t lon) {
  return map(lon, LON_WEST, LON_EAST, 0, MAP_WIDTH);
}

int16_t lat_to_y(int32_t lat) {
  return map(lat, LAT_NORTH, LAT_SOUTH, 0, MAP_HEIGHT);
}

int32_t x_to_lon(int16_t x) {
  return map(x, 0, MAP_WIDTH, LON_WEST, LON_EAST);
}

int32_t y_to_lat(int16_t y) {
  return map(y, 0, MAP_HEIGHT, LAT_NORTH, LAT_SOUTH);
}

/**
 * Calculates Manhattan distance between two points.
 *
 * @param n Length of the array.
 * @param A The array to sort.
 */
int32_t ManhattanDist(int16_t x1, int16_t y1, int16_t x2, int16_t y2) {
  return abs(x1 - x2) + abs(y1 - y2);
}

/**
 * Computes the rating from range [1, 5] given the rating from range [0, 10].
 *
 * @param rating The rating
 */
uint16_t ratingConverter(uint16_t rating) {
  return max(floor((rating+1) / 2), 1);
}
