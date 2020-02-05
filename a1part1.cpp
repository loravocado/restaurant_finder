//-----------------------------------------------------
//
//	Name: Lora Ma
//	ID: 1570935
//	CMPUT 275 : Winter 2020
//	Assignment 2: Restaurants and Pointers: getRestaurantFast
//
//----------------------------------------------------


#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include <SD.h>
#include <TouchScreen.h>

#define SD_CS 10

// physical dimensions of the tft display (# of pixels)
#define DISPLAY_WIDTH  480
#define DISPLAY_HEIGHT 320

// touch screen pins, obtained from the documentaion
#define YP A3 // must be an analog pin, use "An" notation!
#define XM A2 // must be an analog pin, use "An" notation!
#define YM 9  // can be a digital pin
#define XP 8  // can be a digital pin

// dimensions of the part allocated to the map display
#define MAP_DISP_WIDTH (DISPLAY_WIDTH - 60)
#define MAP_DISP_HEIGHT DISPLAY_HEIGHT

#define REST_START_BLOCK 4000000
#define NUM_RESTAURANTS 1066

// calibration data for the touch screen, obtained from documentation
// the minimum/maximum possible readings from the touch point
#define TS_MINX 100
#define TS_MINY 120
#define TS_MAXX 940
#define TS_MAXY 920

// thresholds to determine if there was a touch
#define MINPRESSURE   10
#define MAXPRESSURE 1000

MCUFRIEND_kbv tft;

// a multimeter reading says there are 300 ohms of resistance across the plate,
// so initialize with this to get more accurate readings
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

// different than SD
Sd2Card card;

struct restaurant {
  int32_t lat;
  int32_t lon;
  uint8_t rating; // from 0 to 10
  char name[55];
};


void setup() {
  init();
  Serial.begin(9600);

  // tft display initialization
  uint16_t ID = tft.readID();
  tft.begin(ID);

  tft.fillScreen(TFT_BLACK);
  tft.setRotation(1);

  // SD card initialization for raw reads
  Serial.print("Initializing SPI communication for raw reads...");
  if (!card.init(SPI_HALF_SPEED, SD_CS)) {
    Serial.println("failed! Is the card inserted properly?");
    while (true) {}
  }
  else {
    Serial.println("OK!");
  }

  //initial setup text
  tft.setTextSize(2);

  //draws the red rectangles
  tft.drawRect(420, 0, 60, 160, TFT_RED);
  tft.drawRect(420, 160, 60, 160, TFT_RED);

  //prints the word SLOW
  tft.setCursor(445,40); tft.print("S");
  tft.setCursor(445,60); tft.print("L");
  tft.setCursor(445,80); tft.print("O");
  tft.setCursor(445,100); tft.print("W");

  //prints the word FAST
  tft.setCursor(445,200); tft.print("F");
  tft.setCursor(445,220); tft.print("A");
  tft.setCursor(445,240); tft.print("S");
  tft.setCursor(445,260); tft.print("T");

  //prints initial words onto the screen
  tft.setCursor(0,0); tft.print("RECENT SLOW RUN:");
  tft.setCursor(0,20); tft.print("Not yet run");

  tft.setCursor(0,60); tft.print("SLOW RUN AVG:");
  tft.setCursor(0,80); tft.print("Not yet run");

  tft.setCursor(0,120); tft.print("RECENT FAST RUN:");
  tft.setCursor(0,140); tft.print("Not yet run");

  tft.setCursor(0,180); tft.print("FAST RUN AVG:");
  tft.setCursor(0,200); tft.print("Not yet run");
}


/*
* This function gets restaurants
* @param restIndex: Index of restaurant
* @param restaurant* restPtr: restPtr is using the type restaurant and the
*                    pointer restPtr
*/
void getRestaurant(int restIndex, restaurant* restPtr) {
  uint32_t blockNum = REST_START_BLOCK + restIndex/8;
  restaurant restBlock[8];

  while (!card.readBlock(blockNum, (uint8_t*) restBlock)) {
    Serial.println("Read block failed, trying again.");
  }

  *restPtr = restBlock[restIndex % 8];
}


/*
* This function gets restaurants quicker than the getRestaurant function
* @param restIndex: Index of restaurant
* @param restaurant* restPtr: restPtr is using the type restaurant and the
*                             pointer restPtr
*/
uint32_t current = REST_START_BLOCK;
void getRestaurantFaster(int restIndex, restaurant* restPtr) {
  uint32_t blockNum = REST_START_BLOCK + restIndex/8;
  restaurant restBlock[8];
   if (blockNum != current) {
     while (!card.readBlock(blockNum, (uint8_t*) restBlock)) {
       Serial.println("Read block failed, trying again.");
     }
   }
  *restPtr = restBlock[restIndex % 8];
  current = blockNum;
}

/*
* Reduces the number of overall number of block reads
*/
int fastRunTotal = 0;
int numberOfFastRuns = 0;
int slowAverage = 0;

void fastTest() {
  int currentFastRun;

  //clears small rect where prev words were and writes running
  tft.fillRect(0, 140, 130, 20, TFT_BLACK);
  tft.setCursor(0,140); tft.print("Running...");

  //clears small rect where prev words were and writes Updating
  tft.fillRect(0, 200, 160, 20, TFT_BLACK);
  tft.setCursor(0,200); tft.print("Updating... :)");

  restaurant rest;

  //takes initial time
  int start = millis();

  for (int i = 0; i < NUM_RESTAURANTS; i++) {
    getRestaurantFaster(i, &rest);
  }

  //takes ending time
  int end = millis();

  currentFastRun =  end - start;
  numberOfFastRuns++;
  fastRunTotal = fastRunTotal + currentFastRun;

  //clears small rect where prev words were and writes current run time
  tft.fillRect(0, 140, 130, 20, TFT_BLACK);
  tft.setCursor(0,140); tft.print(currentFastRun); tft.print(" "); tft.print("ms");

  //clears small rect where prev words were and writes average run time
  tft.fillRect(0, 200, 230, 20, TFT_BLACK);
  tft.setCursor(0,200); tft.print(fastRunTotal/numberOfFastRuns); tft.print(" "); tft.print("ms");

}

/*
* Reads the blocks from the SD cards
*/
int slowRunTotal = 0;
int numberOfSlowRuns = 0;
int fastAverage = 0;

void slowTest() {
  int currentSlowRun;

  tft.fillRect(0, 20, 130, 20, TFT_BLACK);
  tft.setCursor(0,20); tft.print("Running...");
  tft.fillRect(0, 80, 130, 20, TFT_BLACK);
  tft.setCursor(0,80); tft.print("Updating... :)");

  restaurant rest;

  //take inital start time
  int start = millis();

  for (int i = 0; i < NUM_RESTAURANTS; i++) {
    getRestaurant(i, &rest);
  }

  //takes end time
  int end = millis();

  currentSlowRun =  end - start;
  numberOfSlowRuns++;
  slowRunTotal = slowRunTotal + currentSlowRun;

  tft.fillRect(0, 20, 130, 20, TFT_BLACK);
  tft.setCursor(0,20); tft.print(currentSlowRun); tft.print(" "); tft.print("ms");

  tft.fillRect(0, 80, 230, 20, TFT_BLACK);
  tft.setCursor(0,80); tft.print(slowRunTotal/numberOfSlowRuns); tft.print(" "); tft.print("ms");

}


void processTouchScreen() {
	TSPoint touchscreen = ts.getPoint();

	pinMode(YP, OUTPUT);
	pinMode(XM, OUTPUT);

	if (touchscreen.z < MINPRESSURE || touchscreen.z > MAXPRESSURE) {
		return;
	}

	int16_t Xpoint = map(touchscreen.y, TS_MINX, TS_MAXX, DISPLAY_WIDTH-1, 0);
	int16_t Ypoint = map(touchscreen.x, TS_MINY, TS_MAXY, DISPLAY_HEIGHT-1, 0);

  //checks for touch area
  if (Xpoint > 420) {
    // Slow area
    if (Ypoint < 160) {
      slowTest();
    }
    // Fast area
    else {
      fastTest();
    }
  }

	delay(200);
}

int main() {
  setup();
  while (true) {
    processTouchScreen();
  }

  Serial.end();

  return 0;
}
