/*

  By:
    * Benjamin Kong -- 1573684
    * Lora Ma       -- 1570935

  CMPUT 275 -- Winter 2020

  Major Assignment #1: Part 1

*/

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include <SPI.h>
#include <SD.h>
#include "lcd_image.h"
#include <TouchScreen.h>

#include "utilities.h"
#include "config.h"

// the "bounds" of the cursor so that it doesn't go off the map
const int min_X = CURSOR_SIZE/2;
const int max_X = DISPLAY_WIDTH - CURSOR_SIZE/2 - 61;
const int min_Y = CURSOR_SIZE/2;
const int max_Y = DISPLAY_HEIGHT - CURSOR_SIZE/2 - 1;

Coord MapPos; // Stores X and Y of current map position
Coord CursorPos; // Stores X and Y of cursor position relative to screen

uint32_t current_block = REST_START_BLOCK; // Stores the current block number
restaurant restBlock[8]; // Stores the current block of restaurants

RestDist restDistances[NUM_RESTAURANTS]; // Stores restaurants based on distance

bool listScreen = false; // temp

MCUFRIEND_kbv tft;
lcd_image_t yegImage = {"yeg-big.lcd", YEG_SIZE, YEG_SIZE};
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);
Sd2Card card;

// Forward declaration for getRestaurant function.
void getRestaurant(int restIndex, restaurant* restPtr);

/**
 * Given an input color, this function redraws the cursor at the current cursor
 * position colored the input color.
 *
 * @param color Color to make the cursor.
 */
void redrawCursor(uint16_t color) {
  tft.fillRect(CursorPos.X - CURSOR_SIZE/2, CursorPos.Y - CURSOR_SIZE/2,
               CURSOR_SIZE, CURSOR_SIZE, color);
}

/**
 * Given the X and Y coordinates to draw, this function will redraw the patch
 * of map that was previously covered by the cursor.
 *
 * @param prev_X X coordinate to redraw the map at
 * @param prev_Y Y coordinate to redraw the map at
 */
void redrawMap(int prev_X, int prev_Y) {
  lcd_image_draw(&yegImage, &tft, MapPos.X + prev_X - CURSOR_SIZE/2 ,
                 MapPos.Y + prev_Y - CURSOR_SIZE/2,
                 prev_X - CURSOR_SIZE/2, prev_Y - CURSOR_SIZE/2,
                 CURSOR_SIZE, CURSOR_SIZE);
}

/*
  tft display initialization.
*/
void tftInitialization() {
  uint16_t ID = tft.readID();
  tft.begin(ID);

  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
}

/**
 * SD card initialization for raw reads.
 */
void SDcardInitialization() {
  Serial.print("Initializing SPI communication for raw reads...");
  if (!card.init(SPI_HALF_SPEED, SD_CS)) {
    Serial.print("failed! Is the card inserted properly?");
    while (true) {}
  } else {
    Serial.print("OK!");
  }

  Serial.print("Initializing SD card...");
	if (!SD.begin(SD_CS)) {
		Serial.println("failed! Is it inserted properly?");
		while (true) {}
	}
	Serial.println("OK!");
}

/**
 * Setup function.
 */
void setup() {
  init();

  Serial.begin(9600);

  pinMode(JOY_SEL, INPUT_PULLUP);

  tftInitialization();
  SDcardInitialization();

  // initial map position
  MapPos.X = YEG_SIZE/2 - (MAP_DISP_WIDTH)/2;
  MapPos.Y = YEG_SIZE/2 - DISPLAY_HEIGHT/2;

  // draws the centre of the Edmonton map, leaving the rightmost 60 columns
  // black
	lcd_image_draw(&yegImage, &tft, MapPos.X, MapPos.Y,
                 0, 0, MAP_DISP_WIDTH, DISPLAY_HEIGHT);

  // initial cursor position is the middle of the screen
  CursorPos.X = MAP_DISP_WIDTH/2;
  CursorPos.Y = DISPLAY_HEIGHT/2;

  // draw the cursor in initial position
  redrawCursor(TFT_RED);

  // Go through all the restaurants (it doesn't read the first 8 restaurants
  // otherwise, for some reason)
  restaurant rest;

  for (int i = 0; i < NUM_RESTAURANTS; i++) {
    getRestaurant(i, &rest);
  }
}

/**
 * Shifts the screen to the new position as required.
 */
void shiftScreen() {
  lcd_image_draw(&yegImage, &tft, MapPos.X, MapPos.Y,
                 0, 0, MAP_DISP_WIDTH, DISPLAY_HEIGHT);
  CursorPos.X = MAP_DISP_WIDTH/2;
  CursorPos.Y = DISPLAY_HEIGHT/2;
  redrawCursor(TFT_RED);
}

/**
 * Fast implementation of getting restaurants.
 *
 * @param restIndex The index of the restaurant to be received.
 * @param restPtr A pointer to the restaurant struct that will be storing the
 *                   received restaurant.
 */
void getRestaurant(int restIndex, restaurant* restPtr) {
  uint32_t blockNum = REST_START_BLOCK + restIndex/8;

  if (blockNum == current_block) {
    *restPtr = restBlock[restIndex % 8];
  }
  else {
    while (!card.readBlock(blockNum, (uint8_t*) restBlock)) {
      Serial.print("Read block failed, trying again.");
    }

    *restPtr = restBlock[restIndex % 8];
    current_block = blockNum;
  }
}

/**
 * Unhighlights the restaurant at the given position in the restaurant list
 * menu.
 */
void unhighlightRest(int pos) {
  restaurant rest;
  getRestaurant(restDistances[pos/15].index, &rest);

  tft.setCursor(0, pos);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.print(rest.name);
}

/**
 * Highlights the currently selected restaurant in the restaurant list menu.
 */
void highlightRest(int pos){
  restaurant rest;
  getRestaurant(restDistances[pos/15].index, &rest);

  tft.setCursor(0, pos);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.print(rest.name);
}

/**
 * Code for the list of restaurants screen. Grabs the 21 closest restaurants
 * and displays them. The currently focused restaurant is highlighted in white.
 *
 * Pressing the joystick selects the restaurant and moves the cursor there,
 * centering it as well.
 */
void restaurantListScreen() {
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  int position = 0; // Position on the screen in terms of pixels

  // Grab the 21 closest restaurants
  for (int i = 0; i < 21; i++) {
    restaurant rest;
    getRestaurant(restDistances[i].index, &rest);
    tft.setCursor(0, 15*i); tft.print(rest.name);
  }
  highlightRest(position);

  while (true) {
    int buttonVal = digitalRead(JOY_SEL);
    int yVal = analogRead(JOY_VERT);

    if (yVal <= 23) {
      unhighlightRest(position);
      position -= 15;
      position = constrain(position, 0, DISPLAY_HEIGHT -20);
      highlightRest(position);
    }
    else if (yVal >= 1000) {
      unhighlightRest(position);
      position += 15;
      position = constrain(position, 0, DISPLAY_HEIGHT -20);
      highlightRest(position);
    }

    if (buttonVal == LOW) {
      restaurant rest;
      getRestaurant(restDistances[position/15].index, &rest);
      MapPos.X = lon_to_x(rest.lon) - DISPLAY_WIDTH/2 - 30;
      MapPos.Y = lat_to_y(rest.lat) - DISPLAY_HEIGHT;
      CursorPos.X = (DISPLAY_WIDTH)/2 - 30;
      CursorPos.Y = DISPLAY_HEIGHT/2;
      break;
    }
  }
}

/**
 * Draws restaurants within the bounds of the display. Restaurants are
 * represented by a 3x3 pixel circle.
 */
void drawNearRestaurants() {
  restaurant rest;

  for (int i = 0; i < NUM_RESTAURANTS; i++) {
    getRestaurant(i, &rest);

    int16_t X = lon_to_x(rest.lon);
    int16_t Y = lat_to_y(rest.lat);

    if (X >= (MapPos.X + 2) && X <= (MapPos.X + MAP_DISP_WIDTH - 2) &&
        Y >= (MapPos.Y + 2) && Y <= (MapPos.Y + MAP_DISP_HEIGHT - 2)) {

      tft.fillCircle(X - MapPos.X, Y - MapPos.Y, 3, TFT_BLUE);

    }
  }
}

/**
 * Grabs all the restaurants and sorts them based on proximity to the cursor.
 * Insertion sort is used to accomplish this.
 */
void sortRestaurants() {
  restaurant rest;

  for (int i = 0; i < NUM_RESTAURANTS; i++) {
    getRestaurant(i, &rest);

    int16_t X1 = lon_to_x(rest.lon);
    int16_t Y1 = lat_to_y(rest.lat);

    int16_t X2 = MapPos.X + CursorPos.X;
    int16_t Y2 = MapPos.Y + CursorPos.Y;

    int32_t manhattanDist = ManhattanDist(X1, Y1, X2, Y2);

    RestDist smallerRest;
    smallerRest.index = i;
    smallerRest.dist = manhattanDist;

    restDistances[i] = smallerRest;
  }

  isort(NUM_RESTAURANTS, restDistances);
}

/**
 * Processes joystick input from the user and moves the cursor accordingly.
 * At the beginning of the function, the joystick input values are recorded.
 * The previous position of the cursor is also recorded.
 *
 * If the joystick is determined to be actually moved, the X and Y cursor
 * positions are updated accordingly and clamped to avoid moving off the map.
 *
 * If there was cursor movement, the patch of map the cursor was previously
 * covering is redrawn and the cursor is redrawn and the new position.
 */
void processJoystick() {
  // read input from the joystick
  int buttonVal = digitalRead(JOY_SEL);
  int xVal = analogRead(JOY_HORIZ);
  int yVal = analogRead(JOY_VERT);

  // the following two variables are used to determine if the cursor moved this
  // frame
  int prev_X = CursorPos.X;
  int prev_Y = CursorPos.Y;

  if (listScreen == false && buttonVal == LOW) {
    listScreen = true;

    tft.fillScreen(TFT_BLACK);
    sortRestaurants();
    restaurantListScreen();
  }
  if (listScreen == true && buttonVal == LOW) {
    listScreen = false;

    tft.fillScreen(TFT_BLACK);

    lcd_image_draw(&yegImage, &tft, MapPos.X, MapPos.Y,
                   0, 0, DISPLAY_WIDTH - 60, DISPLAY_HEIGHT);
    redrawCursor(TFT_RED);
  }

  else if (listScreen == false){
  // check if the joystick is moved
    if (abs(xVal - JOY_CENTER) > JOY_DEADZONE) {
      CursorPos.X -= MAX_SPEED * (xVal - JOY_CENTER) / (JOY_CENTER);
      CursorPos.X = constrain(CursorPos.X, min_X, max_X);
    }

    if (abs(yVal - JOY_CENTER) > JOY_DEADZONE) {
      CursorPos.Y += MAX_SPEED * (yVal - JOY_CENTER) / (JOY_CENTER);
      CursorPos.Y = constrain(CursorPos.Y, min_Y, max_Y);
    }

    if (CursorPos.X == max_X) {
      if (MapPos.X + 2*MAP_DISP_WIDTH < YEG_SIZE) {
        MapPos.X += MAP_DISP_WIDTH;
        shiftScreen();
      }
      else if(MapPos.X + MAP_DISP_WIDTH < YEG_SIZE) {
        MapPos.X += (YEG_SIZE - MapPos.X - MAP_DISP_WIDTH);
        shiftScreen();
      }
    }

    if (CursorPos.X == min_X) {
      if (MapPos.X - MAP_DISP_WIDTH > 0) {
        MapPos.X -= MAP_DISP_WIDTH;
        shiftScreen();
      }
      else if(MapPos.X > 0) {
        MapPos.X = 0;
        shiftScreen();
      }
    }

    if (CursorPos.Y == max_Y) {
      if (MapPos.Y + 2*DISPLAY_HEIGHT < YEG_SIZE) {
        MapPos.Y += DISPLAY_HEIGHT;
        shiftScreen();
      }
      else if (MapPos.Y + DISPLAY_HEIGHT < YEG_SIZE) {
        MapPos.Y += (YEG_SIZE - MapPos.Y - DISPLAY_HEIGHT);
        shiftScreen();
      }
    }

    if (CursorPos.Y == min_Y) {
      if (MapPos.Y - DISPLAY_HEIGHT > 0) {
        MapPos.Y -= DISPLAY_HEIGHT;
        shiftScreen();
      }
      else if (MapPos.Y > 0) {
        MapPos.Y = 0;
        shiftScreen();
      }
    }

    // draw a small patch of the Edmonton map at the old cursor position before
    // drawing a red rectangle at the new cursor position
    if ((prev_X != CursorPos.X) || (prev_Y != CursorPos.Y)) {
      redrawMap(prev_X, prev_Y);
      redrawCursor(TFT_RED);
    }
  }
}

/**
 * Processes touchscreen input. If there was a touch on the map, draw the
 * restaurants that fit on the screen.
 */
void processTouchScreen() {
	TSPoint touchscreen = ts.getPoint();

	pinMode(YP, OUTPUT);
	pinMode(XM, OUTPUT);

	if (touchscreen.z < MINPRESSURE || touchscreen.z > MAXPRESSURE) {
		return;
	}

  int16_t X = map(touchscreen.y, TS_MINX, TS_MAXX, DISPLAY_WIDTH-1, 0);

  if (X < MAP_DISP_WIDTH) {
    drawNearRestaurants();
  }
}

/**
 * Main function of program. Continuously processes touchscreen loop.
 */
int main() {
  setup();

  while (true) {
    processJoystick();
    processTouchScreen();
  }

  Serial.end();

  return 0;
}
