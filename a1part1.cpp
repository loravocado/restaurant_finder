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

MCUFRIEND_kbv tft;

int yegMiddleX = YEG_SIZE/2 - (DISPLAY_WIDTH)/2;
int yegMiddleY = YEG_SIZE/2 - DISPLAY_HEIGHT/2;
int cursorX, cursorY;

// the "bounds" of the cursor so that it doesn't go off the map
int min_X = CURSOR_SIZE/2;
int max_X = DISPLAY_WIDTH - CURSOR_SIZE/2 - 1;
int min_Y = CURSOR_SIZE/2;
int max_Y = DISPLAY_HEIGHT - CURSOR_SIZE/2 - 1;
bool listScreen = false;

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

void restaurantListScreen() {
  uint16_t ID = tft.readID();
  tft.begin(ID);

  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);

  // Display test results
  tft.setCursor(0, 0); tft.print("RECENT SLOW RUN:");
  tft.setCursor(0, 20); tft.print("Not yet run");

  tft.setCursor(0, 60); tft.print("SLOW RUN AVG:");
  tft.setCursor(0, 80); tft.print("Not yet run");

  tft.setCursor(0, 120);  tft.print("RECENT FAST RUN:");
  tft.setCursor(0, 140); tft.print("Not yet run");

  tft.setCursor(0, 180); tft.print("FAST RUN AVG:");
  tft.setCursor(0, 200); tft.print("Not yet run");

  // Slow test button
  tft.drawRect(420, 0, 60, 160, TFT_RED);
  tft.setCursor(445,40); tft.print("S");
  tft.setCursor(445,58); tft.print("L");
  tft.setCursor(445,76); tft.print("O");
  tft.setCursor(445,94); tft.print("W");

  // Fast test button
  tft.drawRect(420, 160, 60, 160, TFT_RED);
  tft.setCursor(445,200); tft.print("F");
  tft.setCursor(445,218); tft.print("A");
  tft.setCursor(445,236); tft.print("S");
  tft.setCursor(445,254); tft.print("T");
}

void shiftScreen() {
  lcd_image_draw(&yegImage, &tft, yegMiddleX, yegMiddleY,
                 0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);
  cursorX = (DISPLAY_WIDTH)/2;
  cursorY = DISPLAY_HEIGHT/2;
  redrawCursor(TFT_RED);
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
    lcd_image_draw(&yegImage, &tft, yegMiddleX, yegMiddleY,
                   0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);
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
      yegMiddleX += DISPLAY_WIDTH;
      yegMiddleX = constrain(yegMiddleX, 0, 2048 - CURSOR_SIZE/2);
      shiftScreen();
    }
    if (cursorX == min_X) {
      yegMiddleX -= DISPLAY_WIDTH;
      yegMiddleX = constrain(yegMiddleX, 0, 2048 - CURSOR_SIZE/2);
      shiftScreen();
    }
    if (cursorY == max_Y) {
      yegMiddleY += DISPLAY_HEIGHT;
      yegMiddleY = constrain(yegMiddleY, 0, 2048 - CURSOR_SIZE/2);
      shiftScreen();
    }
    if (cursorY == min_Y) {
      yegMiddleY -= DISPLAY_HEIGHT;
      yegMiddleY = constrain(yegMiddleY, 0, 2048 - CURSOR_SIZE/2);
      shiftScreen();
    }


    // draw a small patch of the Edmonton map at the old cursor position before
    // drawing a red rectangle at the new cursor position
    if ((prev_X != cursorX) || (prev_Y != cursorY)) {
      redrawMap(prev_X, prev_Y);
      redrawCursor(TFT_RED);
    }
  }
  delay(10);
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

	//    tft.reset();             // hardware reset
  uint16_t ID = tft.readID();    // read ID from display
  Serial.print("ID = 0x");
  Serial.println(ID, HEX);
  if (ID == 0xD3D3) ID = 0x9481; // write-only shield

  // must come before SD.begin() ...
  tft.begin(ID);                 // LCD gets ready to work

  SDcardInitialization();
	tft.setRotation(1);

  tft.fillScreen(TFT_BLACK);

  // draws the centre of the Edmonton map, leaving the rightmost 60 columns
  // black
	lcd_image_draw(&yegImage, &tft, yegMiddleX, yegMiddleY,
                 0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);

  // initial cursor position is the middle of the screen
  cursorX = (DISPLAY_WIDTH)/2;
  cursorY = DISPLAY_HEIGHT/2;

  redrawCursor(TFT_RED);
}

/**
 * Insertion sort.
 *
 * @param n Length of the array.
 * @param A The array to sort.
 */
void isort(int n, int A[]) {
  int i = 1;

  while (i < n) {
    int j = i;

    while (j > 0 && A[j - 1] > A[j]) {
      int temp = A[j - 1];
      A[j - 1] = A[j];
      A[j] = temp;

      j--;
    }

    i++;
  }
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
void fastTest() {
  restaurant rest;

  for (int i = 0; i < NUM_RESTAURANTS; i++) {
    getRestaurant(i, &rest);
  }
}

/**
 * Processes touchscreen input. Runs the fast test or slow test depending
 * on which button is pressed.
 */
void processTouchScreen() {
	TSPoint touchscreen = ts.getPoint();

  // Restore pinMode to output after reading the touch.
    // This is necessary to talk to tft display.
	pinMode(YP, OUTPUT);
	pinMode(XM, OUTPUT);

	if (touchscreen.z < MINPRESSURE || touchscreen.z > MAXPRESSURE) {
		return;
	}

	int16_t Xpoint = map(touchscreen.y, TS_MINX, TS_MAXX, DISPLAY_WIDTH-1, 0);
	//int16_t Ypoint = map(touchscreen.x, TS_MINY, TS_MAXY, DISPLAY_HEIGHT-1, 0);

  //checks for touch area
  if (Xpoint > 420) {
    fastTest();
  }

	delay(200);
}

/**
 * Main function of program. Continuously processes touchscreen loop.
 */
int main() {
  setup();

  // Initial variables
  int screenState = 0; // Initialize at state 0 (aka mode 0)
  RestDist rest_dist[NUM_RESTAURANTS]; 

  while (true) {
    // Mode 0
    if (screenState == 0) {
      processJoystick();
    }
    // Mode 1
    else {
      // Mode 1 stuff
    }
    
  }

  Serial.end();

  return 0;
}
