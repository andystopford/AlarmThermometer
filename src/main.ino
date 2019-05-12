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
MCP7940_Class MCP7940;
const uint8_t SPRINTF_BUFFER_SIZE = 32;
char timeBuff[SPRINTF_BUFFER_SIZE]; // Buffer for sprintf()/sscanf()
max1720x gauge;
//#define TRG 120
//char batt[7];
//Display

enum SwitchStates: byte {IS_OPEN, IS_RISING, IS_CLOSED, IS_FALLING} ;
SwitchStates switchMenu = IS_OPEN;
SwitchStates switchSel = IS_OPEN;
SwitchStates switchUp = IS_OPEN;
SwitchStates switchDown = IS_OPEN;

enum DisplayModes: byte {SHOW_MAIN, SHOW_MENU, TEMP_MENU, SET_UP_TEMP,
	 UP_ENABLE, SET_DOWN_TEMP, DOWN_ENABLE, TIMER_MENU, SET_HOUR, SET_MIN,
	  SETTINGS_MENU};
DisplayModes displayMode = SHOW_MAIN;
enum UpDown: byte {UP, DOWN};
UpDown upDown;

byte highlighted = 0;
bool highlightToggled = false;
byte upTemp = 78;
byte downTemp = 30;

bool timerEnabled = false;

volatile bool itr = false;

static byte sec;		//Do these need to be static?
static byte min;
static byte hr;
byte AlMin = 0;
byte AlHr = 0;

void alarmint()
{
	// Just set flag so main "thread" can act on it
	itr = true;
}

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
  tft.setTextColor(CYAN);
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

//////////////////////////////////////////////////////////////////////////////
void loop()
	{
		menuSwitch();
		selectSwitch();
		upSwitch();
		downSwitch();
		setDisplayMode();
		// Debugging:
		tft.setTextColor(BLUE, BLACK);
		tft.setCursor(0, 85);
		tft.println(highlighted);
	}

//////////////////////////////////////////////////////////////////////////////
void setDisplayMode()
	{
		switch (displayMode)
      {
        case SHOW_MAIN:
          {
            drawTime();
    				showMode();	// should be 0
            if (switchMenu == IS_FALLING)
              {
								tft.fillScreen(BLACK);
                displayMode = SHOW_MENU;
								drawMenu();
              }
						break;
          }

        case SHOW_MENU:
          {
    				readNav(2);	//Check for menu selection
						if (highlightToggled == true)
							{
								drawMenu();
								highlightToggled = false;
							}
    				showMode();	// should be 1
            if (switchMenu == IS_FALLING)
              {
								tft.fillScreen(BLACK);
                displayMode = SHOW_MAIN;
								break;
              }
						if (switchSel == IS_FALLING)
							{
								if (highlighted == 0)
									{
										tft.quickFill(BLACK);
										displayMode = TEMP_MENU;
										highlighted = 0;
										drawTempMenu();
										break;
									}
								if (highlighted == 1)
									{
										tft.quickFill(BLACK);
										displayMode = TIMER_MENU;
										highlighted = 0;
										drawTimerMenu();
										break;
									}
							}
						break;
          }

				case TEMP_MENU:
					{
						readNav(1);	// Select up or down temp
						if (highlightToggled)
							{
								drawTempMenu();
								highlightToggled = false;
							}
						showMode();	//should be 2
						if (switchMenu == IS_FALLING)
              {
								tft.fillScreen(BLACK);
                displayMode = SHOW_MENU;
								drawMenu();
								break;
              }
						if (switchSel == IS_FALLING)
							{
								if (highlighted == 0)
									{
										displayMode = SET_UP_TEMP;
										break;
									}
								if (highlighted == 1)
									{
										displayMode = SET_DOWN_TEMP;
										break;
									}
							}
						break;
					}

				case SET_UP_TEMP:
					{
						showMode();	//should be 3
						highlightUpTemp();
						if (switchSel == IS_FALLING)
							{
								displayMode = TEMP_MENU;
								drawTempMenu();
								showMode();	//should be 2
								break;
							}
						else if (switchUp == IS_CLOSED)
							{
								incrTemp();
								break;
							}
						else if (switchDown == IS_CLOSED)
							{
								decrTemp();
								break;
							}
						else if (switchMenu == IS_FALLING)
							{
								tft.quickFill(BLACK);
								//menuLevel = 1;
								displayMode = SHOW_MENU;
								break;
							}
						break;
					}

				case SET_DOWN_TEMP:
					{
						showMode();	//should be 5
						highlightDownTemp();
						if (switchSel == IS_FALLING)
							{
								displayMode = TEMP_MENU;
								drawTempMenu();
								showMode();	//should be 2
								break;
							}
						else if (switchUp == IS_CLOSED)
							{
								incrTemp();
								break;
							}
						else if (switchDown == IS_CLOSED)
							{
								decrTemp();
								break;
							}
						else if (switchMenu == IS_FALLING)
							{
								tft.quickFill(BLACK);
								displayMode = SHOW_MENU;
								break;
							}
						break;
					}

				case TIMER_MENU:
					{
						readNav(2);
						if (highlightToggled)
							{
								drawTimerMenu();
								highlightToggled = false;
							}
						showMode();	//should be 7
						if (switchMenu == IS_FALLING)
              {
								tft.fillScreen(BLACK);
                displayMode = SHOW_MENU;
								drawMenu();
								break;
              }
						if (switchSel == IS_FALLING)
							{
								if (highlighted == 0)
									{
										displayMode = SET_HOUR;
										break;
									}
								if (highlighted == 1)
									{
										displayMode = SET_MIN;
										break;
									}
								if (highlighted == 2)
									{
										byte t = 't';
										enable(t);
										drawTimerMenu();
										showMode();	//should be 7
										break;
									}
								}
							break;
				 	}

				case SET_HOUR:
					{
						showMode();	//should be 8
						highlightSetHour();
						if (switchSel == IS_FALLING)
							{
								displayMode = TIMER_MENU;
								drawTimerMenu();
								showMode();	//should be 7
								break;
							}
						else if (switchUp == IS_CLOSED)
							{
								incrTime();
								break;
							}
						else if (switchDown == IS_CLOSED)
							{
								decrTime();
								break;
							}
						else if (switchMenu == IS_FALLING)
							{
								tft.quickFill(BLACK);
								//menuLevel = 1;
								displayMode = SHOW_MENU;
								break;
							}
						break;
					}

				case SET_MIN:
					{
						showMode();	//should be 9
						highlightSetMin();
						if (switchSel == IS_FALLING)
							{
								displayMode = TIMER_MENU;
								drawTimerMenu();
								showMode();	//should be 7
								break;
							}
						else if (switchUp == IS_CLOSED)
							{
								incrTime();
								break;
							}
						else if (switchDown == IS_CLOSED)
							{
								decrTime();
								break;
							}
						else if (switchMenu == IS_FALLING)
							{
								tft.quickFill(BLACK);
								//menuLevel = 1;
								displayMode = SHOW_MENU;
								break;
							}
						break;
					}
      }
	}

//////////////////////////////////////////////////////////////////////////////
void menuSwitch()
{
  int pin = digitalRead(MENU_PIN);
  switch (switchMenu)
		{
	    case IS_OPEN:
				{																					// Pin is LOW
					if(pin == HIGH) 										// If pin goes HIGH
					switchMenu = IS_RISING;  							// Change state to IS_RISING
					break;
				}
	    case IS_RISING:															// Switch is closing
				{
					switchMenu = IS_CLOSED; 								// Switch has closed
					break;
				}

	    case IS_CLOSED:
				{ 																				// Pin is HIGH
					if(pin == LOW)  										// If pin goes LOW
					switchMenu = IS_FALLING; 							// Change state to IS_FALLING
					break;
				}
	    case IS_FALLING:													// Switch is opening
				{
					switchMenu = IS_OPEN;    							// Switch has opened
					break;
				}
	  }
}

////////////////////////////////////////////////////////////////////////////////
void selectSwitch()
{
  int pin = digitalRead(SEL_PIN);
  switch (switchSel)
		{
	    case IS_OPEN:
				{																					// Pin was LOW
					if(pin == HIGH) 										// If pin goes HIGH
					switchSel = IS_RISING;  							// Change state to IS_RISING
					break;
				}
	    case IS_RISING:
				{
					switchSel = IS_CLOSED; 								// Switch has closed
					break;
				}

	    case IS_CLOSED:
				{ 																				// Pin was HIGH
					if(pin == LOW)  										// If pin goes LOW
					switchSel = IS_FALLING; 							// Change state to IS_FALLING
					break;
				}
	    case IS_FALLING:
				{
					switchSel = IS_OPEN;    							// Switch has opened
					break;
				}
	  }
}

////////////////////////////////////////////////////////////////////////////////

void upSwitch()
{
  int pin = digitalRead(UP_PIN);
  switch (switchUp)
		{
	    case IS_OPEN:
				{																					// Pin is LOW
					if(pin == HIGH) 										// If pin goes HIGH
					switchUp = IS_RISING;  							// Change state to IS_RISING
					break;
				}
	    case IS_RISING:															// Switch is closing
				{
					switchUp = IS_CLOSED; 								// Switch has closed
					break;
				}

	    case IS_CLOSED:
				{ 																				// Pin is HIGH
					if(pin == LOW)  										// If pin goes LOW
					switchUp = IS_FALLING; 							// Change state to IS_FALLING
					break;
				}
	    case IS_FALLING:													// Switch is opening
				{
					switchUp = IS_OPEN;    							// Switch has opened
					break;
				}
	  }
}

//////////////////////////////////////////////////////////////////////////////
void downSwitch()
{
  int pin = digitalRead(DOWN_PIN);
  switch (switchDown)
		{
	    case IS_OPEN:
				{																					// Pin is LOW
					if(pin == HIGH) 										// If pin goes HIGH
					switchDown = IS_RISING;  							// Change state to IS_RISING
					break;
				}
	    case IS_RISING:															// Switch is closing
				{
					switchDown = IS_CLOSED; 								// Switch has closed
					break;
				}

	    case IS_CLOSED:
				{ 																				// Pin is HIGH
					if(pin == LOW)  										// If pin goes LOW
					switchDown = IS_FALLING; 							// Change state to IS_FALLING
					break;
				}
	    case IS_FALLING:													// Switch is opening
				{
					switchDown = IS_OPEN;    							// Switch has opened
					break;
				}
	  }
}

///////////////////////////////////////////////////////////////////////////////
void showMode()
	{
		tft.setTextColor(BLUE, BLACK);
		tft.setCursor(0, 100);
		tft.print("Mode ");
		tft.print(displayMode);
	}

void readNav(int max)
	{
		// Reads inputs to up and down buttons
		if (digitalRead(DOWN_PIN) == HIGH)
			{
				upDown = DOWN;
				toggleHighLight(max);
			}
		if (digitalRead(UP_PIN) == HIGH)
			{
				upDown = UP;
				toggleHighLight(max);
			}
	}

void toggleHighLight(int max)
	{
		highlightToggled = true;
		switch (upDown)
			{
				case UP:
					{
						if (highlighted == 0)
							{
								highlighted = max;
							}
						else
							{
								highlighted -= 1;
							}
						break;
					}
				case DOWN:
					{
						if (highlighted == max)
							{
								highlighted = 0;
							}
						else
							{
								highlighted += 1;
							}
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
		if (highlighted == 0)
			{
				tft.setTextColor(GREEN, RED);
			}
		else
			{
				tft.setTextColor(GREEN, BLACK);
			}
    tft.print("Up");
		tft.setCursor(100, 22);
		tft.setTextColor(GREEN, BLACK);
		tft.print(upTemp);
		tft.setCursor(0, 40);
		if (highlighted == 1)
			{
				tft.setTextColor(GREEN, RED);
			}
		else
			{
				tft.setTextColor(GREEN, BLACK);
			}
    tft.print("Down");
		tft.setCursor(100, 40);
		tft.setTextColor(GREEN, BLACK);
    tft.print(downTemp);
	}

void highlightUpTemp()
	{
		tft.setTextColor(GREEN, BLACK);
		tft.setCursor(0, 22);
		tft.print("Up");
		tft.setTextColor(GREEN, RED);
		tft.setCursor(100, 22);
		tft.print(upTemp);
	}

void highlightDownTemp()
	{
		tft.setTextColor(GREEN, BLACK);
		tft.setCursor(0, 40);
		tft.print("Down");
		tft.setTextColor(GREEN, RED);
		tft.setCursor(100, 40);
		tft.print(downTemp);
	}

void incrTemp()
	{
		if (highlighted == 0)
		{
			if (upTemp < 99)
				{
					tft.setCursor(100, 22);
					upTemp += 1;
					tft.print(upTemp);
				}
		}
		else if (highlighted == 1)
		{
			if (downTemp < 99)
				{
					tft.setCursor(100, 40);
					downTemp += 1;
					tft.print(downTemp);
				}
		}
	}

void decrTemp()
	{
		if (highlighted == 0)
		{
			if (upTemp  > 1)
				{
					tft.setCursor(100, 22);
					upTemp -= 1;
					tft.print(upTemp);
				}
		}
		else if (highlighted == 1)
		{
			if (downTemp > 1)
				{
					tft.setCursor(100, 40);
					downTemp -= 1;
					tft.print(downTemp);
				}
		}
	}

void drawTimerMenu()
	{
		tft.setCursor(0, 0);
		tft.setTextColor(BLUE, BLACK);
		tft.println(" Set Timer");
		tft.setCursor(0, 22);	// Prepare to draw hours
		if (highlighted == 0)
			{
				tft.setTextColor(GREEN, RED);
			}
		else
			{
				tft.setTextColor(GREEN, BLACK);
			}
		tft.print("Hours");
		tft.setCursor(100, 22);
		tft.setTextColor(GREEN, BLACK);
		if (AlHr < 10)
			{
				tft.print(0);
			}
    tft.print(AlHr);

		tft.setCursor(0, 40);	// Prepare to draw minutes
		if (highlighted == 1)
			{
				tft.setTextColor(GREEN, RED);
			}
		else
			{
				tft.setTextColor(GREEN, BLACK);
			}
		tft.print("Mins");
		tft.setCursor(100, 40);
		tft.setTextColor(GREEN, BLACK);
		if (AlMin < 10)
			{
				tft.print(0);
			}
    tft.print(AlMin);

		tft.setCursor(0, 58);	// Prepare to draw enabled switch
		if (highlighted == 2)
			{
				tft.setTextColor(GREEN, RED);
			}
		else
			{
				tft.setTextColor(GREEN, BLACK);
			}
		tft.print("Enabled");
		tft.drawRoundRect(105, 58, 16, 16, 2, GREEN);
		if (timerEnabled == true)
			{
				tft.fillRoundRect(106, 59, 14, 14, 2, RED);
			}
	}

void highlightSetHour()
	{
		tft.setTextColor(GREEN, BLACK);
		tft.setCursor(0, 22);
		tft.print("Hours");
		tft.setTextColor(GREEN, RED);
		tft.setCursor(100, 22);
		if (AlHr > 12)
			{
				AlHr = 0;
			}
		if (AlHr < 10)
			{
				tft.print(0);
			}
		tft.print(AlHr);
	}

void highlightSetMin()
	{
		tft.setTextColor(GREEN, BLACK);
		tft.setCursor(0, 40);
		tft.print("Mins");
		tft.setTextColor(GREEN, RED);
		tft.setCursor(100, 40);
		if (AlMin > 59)
			{
				AlMin = 0;
			}
		if (AlMin < 10)
			{
				tft.print(0);
			}
		tft.print(AlMin);
	}

void incrTime()
	{
		if (highlighted == 0)	//increment SET_HOUR
			{
				tft.setCursor(100, 22);
				AlHr += 1;
				if (AlHr > 12)
					{
						AlHr = 0;
					}
				if (AlHr < 10)
					{
						tft.print(0);
					}
		    tft.print(AlHr);
			}
		else if (highlighted == 1)
			{
				tft.setCursor(100, 40);
				AlMin += 1;
				if (AlMin > 59)
					{
						AlMin = 0;
					}
				if (AlMin < 10)
					{
						tft.print(0);
					}
				tft.print(AlMin);
			}
	}

void decrTime()
	{
		if (highlighted == 0)	//increment SET_HOUR
			{
				tft.setCursor(100, 22);
				tft.setTextColor(GREEN, RED);
				AlHr -= 1;
				if (AlHr < 10)
					{
						tft.print(0);
					}
		    tft.print(AlHr);
			}
		else if (highlighted == 1)
			{
				tft.setCursor(100, 40);
				tft.setTextColor(GREEN, RED);
				AlMin -= 1;
				if (AlMin < 10)
					{
						tft.print(0);
					}
				tft.print(AlMin);
			}
	}

///////////////////////////////////////////////////////////////////////////////

void enable(byte mode)
	{
		if (mode == 't')
			{
				if (timerEnabled == true)
					{
						tft.drawRoundRect(105, 58, 16, 16, 2, GREEN);
						tft.fillRoundRect(106, 59, 14, 14, 2, BLACK);
						timerEnabled = false;
					}
				else
					{
						tft.drawRoundRect(105, 58, 16, 16, 2, GREEN);
						tft.fillRoundRect(106, 59, 14, 14, 2, RED);
						timerEnabled = true;
					}
			}
	}

///////////////////////////////////////////////////////////////////////////////
