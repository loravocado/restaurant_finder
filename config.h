#pragma once

#define SD_CS 10

#define DISPLAY_WIDTH  480
#define DISPLAY_HEIGHT 320
#define YEG_SIZE 2048

#define YP A3
#define XM A2
#define YM 9
#define XP 8

//Map constants
#define MAP_DISP_WIDTH (DISPLAY_WIDTH )
#define MAP_DISP_HEIGHT DISPLAY_HEIGHT

#define REST_START_BLOCK 4000000
#define NUM_RESTAURANTS 1066

//TS constants
#define TS_MINX 100
#define TS_MINY 120
#define TS_MAXX 940
#define TS_MAXY 920

#define MINPRESSURE   10
#define MAXPRESSURE 1000

//Cursor constants
#define CURSOR_SIZE 9
#define MAX_SPEED 8

//Joystick constants
#define JOY_VERT  A9 // should connect A9 to pin VRx
#define JOY_HORIZ A8
#define JOY_SEL   53
#define JOY_CENTER   512
#define JOY_DEADZONE 64
