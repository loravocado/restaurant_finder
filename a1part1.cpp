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

int yegMiddleX = YEG_SIZE/2 - (DISPLAY_WIDTH - 60)/2;
int yegMiddleY = YEG_SIZE/2 - DISPLAY_HEIGHT/2;
int cursorX, cursorY;

// the "bounds" of the cursor so that it doesn't go off the map
int min_X = CURSOR_SIZE/2;
int max_X = DISPLAY_WIDTH - CURSOR_SIZE/2 - 61;
int min_Y = CURSOR_SIZE/2;
int max_Y = DISPLAY_HEIGHT - CURSOR_SIZE/2 - 1;
bool listScreen = false;

MCUFRIEND_kbv tft;

lcd_image_t yegImage = {"yeg-big.lcd", YEG_SIZE, YEG_SIZE};

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

Sd2Card card;

// restBlock stores the current block in a global variable so
// that we don't need to read the SD card each time if two entries are in the
// same block.
restaurant restBlock[8];

// Stores the current block number.
uint32_t current_block = REST_START_BLOCK;

/**
 * Given an input color, this function redraws the cursor at the current cursor
 * position colored the input color.
 *
 * @param color  Color to make the cursor.
 */
void redrawCursor(uint16_t color) {
  tft.fillRect(cursorX - CURSOR_SIZE/2, cursorY - CURSOR_SIZE/2,
               CURSOR_SIZE, CURSOR_SIZE, color);
}

/**
 * Given the X and Y coordinates to draw, this function will redraw the patch
 * of map that was previously covered by the cursor.
 *
 * @param prev_X  X coordinate to redraw the map at
 * @param prev_Y  Y coordinate to redraw the map at
 */
void redrawMap(int prev_X, int prev_Y) {
  lcd_image_draw(&yegImage, &tft, yegMiddleX + prev_X - CURSOR_SIZE/2 ,
                 yegMiddleY + prev_Y - CURSOR_SIZE/2,
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

  pinMode(JOY_SEL, INPUT_PULLUP);

  Serial.begin(9600);

  tftInitialization();
  SDcardInitialization();

  // draws the centre of the Edmonton map, leaving the rightmost 60 columns
  // black
	lcd_image_draw(&yegImage, &tft, yegMiddleX, yegMiddleY,
                 0, 0, DISPLAY_WIDTH - 60, DISPLAY_HEIGHT);

  // initial cursor position is the middle of the screen
  cursorX = (DISPLAY_WIDTH - 60)/2;
  cursorY = DISPLAY_HEIGHT/2;

  redrawCursor(TFT_RED);
}

void restaurantListScreen() {
  tft.fillScreen(TFT_BLACK);

  // Grab the 21 closest restaurants

  // Display them in a list
}

void shiftScreen() {
  lcd_image_draw(&yegImage, &tft, yegMiddleX, yegMiddleY,
                 0, 0, DISPLAY_WIDTH - 60, DISPLAY_HEIGHT);
  cursorX = (DISPLAY_WIDTH)/2 - 30;
  cursorY = DISPLAY_HEIGHT/2;
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

  // If the block we need to look in is the same as the one previously
    // searched, we can just look at the block (it is a global
    // variable).
  if (blockNum == current_block) {
    *restPtr = restBlock[restIndex % 8];

  // Otherwise, we get the new block.
  } else {
    while (!card.readBlock(blockNum, (uint8_t*) restBlock)) {
      Serial.print("Read block failed, trying again.");
    }

    *restPtr = restBlock[restIndex % 8];
    current_block = blockNum;
  }
}

/**
 * Temp code
 */
void drawNearRestaurants() {
  restaurant rest; 

  for (int i = 0; i < NUM_RESTAURANTS; i++) {
    getRestaurant(i, &rest);

    int32_t X = lon_to_x(rest.lon);
    int32_t Y = lat_to_y(rest.lat);

    
  }
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
  int prev_X = cursorX;
  int prev_Y = cursorY;

  if (listScreen == false && buttonVal == LOW) {
    listScreen = true;
    restaurantListScreen();
  }
  else if (listScreen == true && buttonVal == LOW) {
    listScreen = false;
    tft.fillScreen(TFT_BLACK);

    lcd_image_draw(&yegImage, &tft, yegMiddleX, yegMiddleY,
                   0, 0, DISPLAY_WIDTH - 60, DISPLAY_HEIGHT);
    redrawCursor(TFT_RED);
  }
  else {
  // check if the joystick is moved
    if (abs(xVal - JOY_CENTER) > JOY_DEADZONE) {
      cursorX -= MAX_SPEED * (xVal - JOY_CENTER) / (JOY_CENTER);
      cursorX = constrain(cursorX, min_X, max_X);
    }

    if (abs(yVal - JOY_CENTER) > JOY_DEADZONE) {
      cursorY += MAX_SPEED * (yVal - JOY_CENTER) / (JOY_CENTER);
      cursorY = constrain(cursorY, min_Y, max_Y);
    }

    if (cursorX == max_X) {
      if (yegMiddleX + 2*DISPLAY_WIDTH - 60 < YEG_SIZE) {
        yegMiddleX += DISPLAY_WIDTH - 60;
        shiftScreen();
      }
      else if(yegMiddleX + DISPLAY_WIDTH - 60 < YEG_SIZE) {
        yegMiddleX += (YEG_SIZE - yegMiddleX - DISPLAY_WIDTH + 60 );
        shiftScreen();
      }
    }

    if (cursorX == min_X) {
      if (yegMiddleX - DISPLAY_WIDTH > 0) {
        yegMiddleX -= DISPLAY_WIDTH + 60;
        shiftScreen();
      }
      else if(yegMiddleX > 0) {
        yegMiddleX = 0;
        shiftScreen();
      }
    }

    if (cursorY == max_Y) {
      if (yegMiddleY + 2*DISPLAY_HEIGHT < YEG_SIZE) {
        yegMiddleY += DISPLAY_HEIGHT;
        shiftScreen();
      }
      else if (yegMiddleY + DISPLAY_HEIGHT < YEG_SIZE) {
        yegMiddleY += (YEG_SIZE - yegMiddleY - DISPLAY_HEIGHT);
        shiftScreen();
      }
    }

    if (cursorY == min_Y) {
      if (yegMiddleY - DISPLAY_HEIGHT > 0) {
        yegMiddleY -= DISPLAY_HEIGHT;
        shiftScreen();
      }
      else if (yegMiddleY > 0) {
        yegMiddleY = 0;
        shiftScreen();
      }

    }

    // draw a small patch of the Edmonton map at the old cursor position before
    // drawing a red rectangle at the new cursor position
    if ((prev_X != cursorX) || (prev_Y != cursorY)) {
      redrawMap(prev_X, prev_Y);
      redrawCursor(TFT_RED);
    }
  }
}

/**
 * Processes touchscreen input. 
 */
void processTouchScreen() {
	TSPoint touchscreen = ts.getPoint();

	pinMode(YP, OUTPUT);
	pinMode(XM, OUTPUT);

	if (touchscreen.z < MINPRESSURE || touchscreen.z > MAXPRESSURE) {
		return;
	}

  Serial.println("Pressed");

	drawNearRestaurants();
}

/**
 * Main function of program. Continuously processes touchscreen loop.
 */
int main() {
  setup();

  int mode = 0;
  RestDist restDistances[NUM_RESTAURANTS];

  while (true) {
    processJoystick();
    processTouchScreen();

    delay(10);
  }

  Serial.end();

  return 0;
}
