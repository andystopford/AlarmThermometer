#include <Arduino.h>

/***************************************************
   This is a example sketch demonstrating graphic drawing
   capabilities of the SSD1351 library for the 1.5"
   and 1.27" 16-bit Color OLEDs with SSD1351 driver chip

   Pick one up today in the adafruit shop!
   ------> http://www.adafruit.com/products/1431
   ------> http://www.adafruit.com/products/1673

   If you're using a 1.27" OLED, change SCREEN_HEIGHT to 96 instead of 128.

   These displays use SPI to communicate, 4 or 5 pins are required to
   interface
   Adafruit invests time and resources providing this open source code,
   please support Adafruit and open-source hardware by purchasing
   products from Adafruit!

   Written by Limor Fried/Ladyada for Adafruit Industries.
   BSD license, all text above must be included in any redistribution

   The Adafruit GFX Graphics core library is also required
   https://github.com/adafruit/Adafruit-GFX-Library
   Be sure to install it!
 ****************************************************/

#define SCLK_PIN 13
#define MOSI_PIN 11
#define DC_PIN   8
#define CS_PIN   10
#define RST_PIN  9
#define MENU_PIN 4
#define UP_PIN A0
#define DOWN_PIN 5
#define SEL_PIN 7
#define PIN_INT 1 //From RTC

// Color definitions
#define BLACK           0x0000
#define BLUE            0x001F
#define RED             0xF800
#define GREEN           0x07E0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0
#define WHITE           0xFFFF

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include <SPI.h>
//#include <RTCC_MCP7940N.h>
#include <MCP7940.h>
#include <max1720x.h>
//#include <EnableInterrupt.h>
#include "PinChangeInterrupt.h"
//#include "custom.h"

// Option 1: use any pins but a little slower
//Adafruit_SSD1351 tft = Adafruit_SSD1351(SCREEN_WIDTH, SCREEN_HEIGHT, CS_PIN, DC_PIN, MOSI_PIN, SCLK_PIN, RST_PIN);

// Option 2: must use the hardware SPI pins
// (for UNO thats sclk = 13 and sid = 11) and pin 10 must be
// an output. This is much faster - also required if you want
// to use the microSD card (see the image drawing example)
//Adafruit_SSD1351 tft = Adafruit_SSD1351(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI,
//  CS_PIN, DC_PIN, RST_PIN);
Adafruit_SSD1351 tft = Adafruit_SSD1351(CS_PIN, DC_PIN, RST_PIN);

//const uint8_t  MCP7940_MEMORY_SIZE = 64; ///< Number of bytes in memory

//RTCC_MCP7940N rtc;
MCP7940_Class MCP7940;
const uint8_t SPRINTF_BUFFER_SIZE = 32;
char timeBuff[SPRINTF_BUFFER_SIZE]; // Buffer for sprintf()/sscanf()
max1720x gauge;
//#define TRG 120
//char batt[7];
//Display
enum DisplayModes {SHOW_MAIN, SHOW_MENU, TEMP_MENU, SET_UP_TEMP, SET_DOWN_TEMP, TIMER_MENU, SETTINGS_MENU};
DisplayModes displayMode = SHOW_MAIN;
enum UpDown: byte {UP, DOWN};
UpDown upDown;
uint8_t menuLevel;
uint8_t highlighted = 0;
uint8_t upTemp = 78;
uint8_t downTemp = 30;

volatile bool itr = false;
//int buttonState = 0;

void alarmint()
{
	// Just set flag so main "thread" can act on it
	itr = true;
}

static uint8_t sec;
static uint8_t min;
static uint8_t hr;

void setup()
{
  pinMode(MENU_PIN, INPUT_PULLUP);
	pinMode(UP_PIN, INPUT_PULLUP);
	pinMode(DOWN_PIN, INPUT_PULLUP);
	pinMode(SEL_PIN, INPUT_PULLUP);
  //attachPCINT(DOWN_PIN, testDown, IS_FALLING);
  Wire.begin();
  tft.begin();
  // You can optionally rotate the display by running the line below.
  // Note that a value of 0 means no rotation, 1 means 90 clockwise,
  // 2 means 180 degrees clockwise, and 3 means 270 degrees clockwise.
  //tft.setRotation(2);
  tft.setCursor(30, 50);
  tft.setTextColor(CYAN);
  tft.print("Bollox");

  gauge.reset(); // Resets MAX1720x
  //Serial.println("gauge");
	// RTC
	while (!MCP7940.begin())
		{ // Initialize RTC communications    //
			tft.fillScreen(BLACK);
			tft.setCursor(0, 0);
			tft.println(F("Unable to find MCP7940M"));
			delay(2000);
	 	}
 while (!MCP7940.deviceStatus()) // Turn oscillator on if necessary
  {
		tft.fillScreen(BLACK);
		tft.setCursor(0, 0);
    tft.println(F("Oscillator is off, turning it on."));
    bool deviceStatus = MCP7940.deviceStart(); // Start oscillator and return state
    if (!deviceStatus) // If it didn't start
    {
			tft.fillScreen(BLACK);
	  	tft.setCursor(0, 0);
      tft.println(F("Oscillator did not start, trying again."));
      delay(1000);
    }
  }
  tft.fillScreen(BLACK);
  tft.setCursor(0, 0);
  tft.setTextSize(2);
	// Enable interrupt
	pinMode( PIN_INT, INPUT_PULLUP );
  //attachInterrupt( digitalPinToInterrupt(PIN_INT), alarmint, RISING );
	//enableInterrupt( PIN_INT, alarmint, CHANGE );
  //attachPinChangeInterrupt(PIN_INT, alarm, RISING);
  attachPCINT(17, alarmint, CHANGE); //n.b. Serial must not be used with TX pin
}

///////////////////////////////////////////////////////////////////////////////
void loop()
  {
		if (digitalRead(MENU_PIN) == HIGH)
			{
				tft.quickFill(BLACK);
				setDisplayMode();
			}

		if (displayMode == SHOW_MAIN)
			{
				drawTime();
				showMode();	// should be 0
			}

		else if (displayMode == SHOW_MENU)
			{
				menuLevel = 1;
        drawMenu();
				readNav();	//Check for menu selection
				showMode();	// should be 1

				if (digitalRead(SEL_PIN) == HIGH)
					{
						if (highlighted == 0)
							{
								displayMode = TEMP_MENU;
								tft.quickFill(BLACK);
								highlighted = 0;
								drawTempMenu();
							}
						if (highlighted == 1)
							{
								displayMode == TIMER_MENU;
								tft.quickFill(BLACK);
								highlighted = 0;
								drawTimerMenu();
							}
					}
			 }

		else if (displayMode == TEMP_MENU)
			{
				if (digitalRead(MENU_PIN) == HIGH)
					{
						tft.quickFill(BLACK);
						displayMode = SHOW_MENU;
						menuLevel = 1;
					}
				showMode();	//should be 2
				readNav();
				drawTempMenu();
				setDisplayMode();
				}

		else if (displayMode == SET_UP_TEMP || SET_DOWN_TEMP)
			{
				showMode();	//should be 3 or 4
				if (digitalRead(SEL_PIN) == HIGH)
					{
						displayMode = TEMP_MENU;
						showMode();	//should be 2
					}
				else if (digitalRead(UP_PIN) == HIGH)
					{
						incrTemp();
					}
				else if (digitalRead(DOWN_PIN) == HIGH)
					{
						decrTemp();
					}
				else if (digitalRead(MENU_PIN) == HIGH)
					{
						tft.quickFill(BLACK);
						menuLevel = 1;
						displayMode = SHOW_MENU;
						//setDisplayMode();
					}
			}

		else if (displayMode == TIMER_MENU)
			{
				setDisplayMode();
				//showMode(); 	// should be 5
				//readNav();
				//drawTempMenu();
			}
  }

void showMode()
	{
		tft.setTextColor(GREEN, BLACK);
		tft.setCursor(0, 70);
		tft.print(displayMode);
	}

void readNav()
	{
		if (digitalRead(DOWN_PIN) == HIGH)
			{
				upDown = DOWN;
				toggleHighLight();
			}
		if (digitalRead(UP_PIN) == HIGH)
			{
				upDown = UP;
				toggleHighLight();
			}
	}

/////////////////////////////////////////////////////////////////////////////
void setDisplayMode()
	{
		// if main window dislayed, switch to menu and vice versa
		switch (displayMode)
			{
				case SHOW_MAIN:
					{
						displayMode = SHOW_MENU;
						break;
					}
				case SHOW_MENU:
					{
						displayMode = SHOW_MAIN;
						break;
					}
				case TEMP_MENU:
					{
						if (digitalRead(SEL_PIN) == HIGH)
							{
								if (highlighted == 0)
									{
										displayMode = SET_UP_TEMP;
										tft.quickFill(BLACK);
										//highlighted = 0;
										drawTempMenu();
									}
								else if (highlighted == 1)
									{
										displayMode = SET_DOWN_TEMP;
										tft.quickFill(BLACK);
										//highlighted = 0;
										drawTempMenu();
									}
							}
					}

					case TIMER_MENU:
						{
							showMode(); 	// should be 5
							readNav();
							drawTempMenu();
						}
				}
	}

/////////////////////////////////////////////////////////////////////////////
void toggleHighLight()
	{
		switch (upDown)
			{
				case UP:
					{
						highlighted -= 1;
						break;
					}
				case DOWN:
					{
						highlighted += 1;
						break;
					}
			}
	}

//////////////////////////////////////////////////////////////////////////////
void alarm (void) {
    tft.setCursor(0,30);
    tft.setTextColor(RED);
		tft.println( "ALARM" );
		//rtc.ClearAlarm1Flag();
		itr = false;
    tft.setCursor(0, 0);
}


///////////////////////////////////////////////////////////////////////////////
void drawMenu()
  {
    tft.setCursor(0, 0);
    if (highlighted == 0)
      {
        tft.setTextColor(GREEN, RED);
        tft.println("Set Temp");
        tft.setTextColor(GREEN, BLACK);
        tft.println("Set Timer");
        tft.println("Settings");
      }
    else if (highlighted == 1)
      {
        tft.setTextColor(GREEN, BLACK);
        tft.println("Set Temp");
        tft.setTextColor(GREEN, RED);
        tft.println("Set Timer");
        tft.setTextColor(GREEN, BLACK);
        tft.println("Settings");
      }
    else if (highlighted == 2)
    {
      tft.setTextColor(GREEN, BLACK);
      tft.println("Set Temp");
      tft.println("Set Timer");
      tft.setTextColor(GREEN, RED);
      tft.println("Settings");
    }
  }

void drawTempMenu()
	{
		tft.setCursor(0, 0);
		tft.setTextColor(BLUE, BLACK);
		tft.println(" Set Temp");

		tft.setCursor(0, 22);
    tft.setTextColor(GREEN, BLACK);
    tft.print("Up");
		tft.setCursor(100, 22);
		if (highlighted == 0)
			{
				tft.setTextColor(GREEN, RED);
			}
		else
			{
				tft.setTextColor(GREEN, BLACK);
			}
		tft.print(upTemp);
		tft.setCursor(0, 40);
    tft.setTextColor(GREEN, BLACK);
    tft.print("Down");
		tft.setCursor(100, 40);
		if (highlighted == 1)
			{
				tft.setTextColor(GREEN, RED);
			}
		else
			{
				tft.setTextColor(GREEN, BLACK);
			}
    tft.print(downTemp);
	}

void incrTemp()
	{
		tft.setTextColor(RED, BLACK);
		if (highlighted == 0)
		{
			tft.setCursor(100, 22);
			upTemp += 1;
			tft.print(upTemp);
		}
		else if (highlighted == 1)
		{
			tft.setCursor(100, 40);
			downTemp += 1;
			tft.print(downTemp);
		}
	}

void decrTemp()
	{
		tft.setTextColor(RED, BLACK);
		if (highlighted == 0)
		{
			tft.setCursor(100, 22);
			upTemp -= 1;
			tft.print(upTemp);
		}
		else if (highlighted == 1)
		{
			tft.setCursor(100, 40);
			downTemp += 1;
			tft.print(downTemp);
		}
	}


void drawTimerMenu()
	{
		uint8_t alarmState =MCP7940_ALM0IF;
		tft.quickFill(BLACK);
		tft.setCursor(0, 0);
		tft.println("alarmState");
		tft.println(alarmState);
	}
///////////////////////////////////////////////////////////////////////////////
void drawTime()
  {
		tft.setCursor(0, 0);
    tft.setTextColor(BLUE, BLACK);
		DateTime now = MCP7940.now();
		sec = now.second();
		min = now.minute();
		hr = now.hour();
		sprintf(timeBuff,"%02d:%02d:%02d", now.hour(), now.minute(), now.second());
		tft.print(timeBuff);
    tft.setCursor(0, 0);
  }

///////////////////////////////////////////////////////////////////////////
void drawtext(char *text, uint16_t color)
{
  tft.setCursor(0,0);
  tft.setTextColor(color);
  tft.print(text);
}
