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
#include <SD.h>
#include <TouchScreen.h>

#define SD_CS 10

#define DISPLAY_WIDTH  480
#define DISPLAY_HEIGHT 320

#define YP A3 
#define XM A2 
#define YM 9 
#define XP 8 

#define MAP_DISP_WIDTH (DISPLAY_WIDTH - 60)
#define MAP_DISP_HEIGHT DISPLAY_HEIGHT

#define REST_START_BLOCK 4000000
#define NUM_RESTAURANTS 1066

#define TS_MINX 100
#define TS_MINY 120
#define TS_MAXX 940
#define TS_MAXY 920

#define MINPRESSURE   10
#define MAXPRESSURE 1000

MCUFRIEND_kbv tft;

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

Sd2Card card;

struct restaurant {
  int32_t lat;
  int32_t lon;
  uint8_t rating; // from 0 to 10
  char name[55];
};

// restBlock stores the current block in a global variable so
// that we don't need to read the SD card each time if two entries are in the 
// same block.
restaurant restBlock[8];

// Stores the current block number.
uint32_t current_block = REST_START_BLOCK;

/**
 * tft display initialization.
 */
void tftInitialization() {
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
}

/**
 * Setup function.
 */
void setup() {
  init();
  Serial.begin(9600);

  tftInitialization();
  SDcardInitialization();
}

/**
 * Example function and function header.
 *
 * @param input Some input.
 * @returns Some output.
 */
int exampleFunction(int input) {
  input++;

  return input;
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

  while (true) {
    processTouchScreen();
  }
  
  Serial.end();

  return 0;
}

