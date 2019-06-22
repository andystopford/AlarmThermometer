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
#define BZR_PIN 6
#define TEMP_PIN 3

// Color definitions
// 24 to 16 bit colour converter: http://drakker.org/convert_rgb565.html
#define BLACK           0x0000
#define BLUE            0x001F
#define DKBLUE					0x000F
#define RED             0xF800
#define DKRED						0x7800
#define GREEN           0x07E0
#define DKGREEN					0x03E0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0
#define WHITE           0xFFFF
#define GREY						0x7BEF		// 50% grey
#define DKGREY					0x39E7

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include <SPI.h>
#include <MCP7940.h>  // RTC
#include <max1720x.h> // Batt Gauge
#include <OneWire.h>
//#include <DallasTemperature.h>
//#include <EnableInterrupt.h>
//#include "PinChangeInterrupt.h"
#include <EEPROM.h>

Adafruit_SSD1351 tft = Adafruit_SSD1351(CS_PIN, DC_PIN, RST_PIN);
MCP7940_Class MCP7940;
const uint8_t SPRINTF_BUFFER_SIZE = 32;
char timeBuff[SPRINTF_BUFFER_SIZE]; // Buffer for sprintf()/sscanf()
max1720x gauge;

/*! ///< Enumeration of MCP7940 alarm types */
enum alarmTypes {matchSeconds,matchMinutes,matchHours,matchDayOfWeek,matchDayOfMonth,Unused1,
  Unused2,matchAll,Unknown};

enum SwitchStates: byte {IS_OPEN, IS_RISING, IS_CLOSED, IS_FALLING} ;
SwitchStates switchMenu = IS_OPEN;
SwitchStates switchSel = IS_OPEN;
SwitchStates switchUp = IS_OPEN;
SwitchStates switchDown = IS_OPEN;

enum DisplayModes: byte {SHOW_MAIN, SHOW_MENU, TEMP_MENU, SET_UP_TEMP,
 SET_DOWN_TEMP, TIMER_MENU, SET_HOUR, SET_MIN, SETTINGS_MENU, SET_DEFUP_TEMP,
 SET_DEFDOWN_TEMP};
DisplayModes displayMode = SHOW_MAIN;
enum UpDown: byte {UP, DOWN};
UpDown upDown;

byte highlighted = 0;
bool highlightToggled = false;
byte defUpTemp;
byte defDownTemp;
byte DefUpTemp_addr = 0; // EEPROM address for storing default value
byte DefDownTemp_addr = 10; // EEPROM address for storing default value
byte upTemp;
byte downTemp;

// Temperature
OneWire oneWire(3);
// declare DS18B20 device address
byte tempSensor[] = {0x28, 0xA1, 0x2F, 0x9A, 0x07, 0x00, 0x00, 0x62};
byte tempC;
unsigned long previousMillis = 0;
unsigned long previousMillisTemp = 0;
unsigned long previousMillisAlarm = 0;
byte buzrState = LOW;

bool mainDisplayed = false;
bool upEnabled = false;
bool downEnabled = false;
bool timerEnabled = false;

byte sec;
byte min;
byte hr;
byte AlMin = 0;
byte AlHr = 0;

//////////////////////////////////////////////////////////////////////////////
void setup()
  {
    pinMode(MENU_PIN, INPUT_PULLUP);
  	pinMode(UP_PIN, INPUT_PULLUP);
  	pinMode(DOWN_PIN, INPUT_PULLUP);
  	pinMode(SEL_PIN, INPUT_PULLUP);
    pinMode(BZR_PIN, OUTPUT);
    digitalWrite(BZR_PIN, LOW);

    //attachPCINT(DOWN_PIN, testDown, IS_FALLING);
    //Wire.begin();
    tft.begin();
    // You can optionally rotate the display by running the line below.
    // Note that a value of 0 means no rotation, 1 means 90 clockwise,
    // 2 means 180 degrees clockwise, and 3 means 270 degrees clockwise.
    tft.setRotation(2);
    tft.setTextColor(CYAN);
    //gauge.reset(); // Resets MAX1720x
    //Serial.println("gauge");
  	// RTC
  	while (!MCP7940.begin())
  		{
        // Initialize RTC communications
  			tft.fillScreen(BLACK);
  			tft.setCursor(0, 0);
  			tft.println(F("Unable to find RTC"));
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
  	// Enable interrupt (MAY NOT BE NEEDED)
  	//pinMode( PIN_INT, INPUT_PULLUP );
    // FOLLOWING MAY NOT BE NEEDED......
    //attachInterrupt( digitalPinToInterrupt(PIN_INT), alarmint, RISING );
  	//enableInterrupt( PIN_INT, alarmint, CHANGE );
    //attachPinChangeInterrupt(PIN_INT, alarm, RISING);
    //attachPCINT(17, alarmint, CHANGE); //n.b. Serial must not be used with TX pin
    EEPROM.get(DefUpTemp_addr, defUpTemp);
    EEPROM.get(DefDownTemp_addr, defDownTemp);
    upTemp = defUpTemp;
    downTemp = defDownTemp;
    oneWire.select(tempSensor);
    oneWire.write(0x1F);  // set 9-bit resolution
    gauge.reset();
  }

//////////////////////////////////////////////////////////////////////////////
void loop()
  {
    setDisplayMode();
		menuSwitch();
		selectSwitch();
    upSwitch();
    downSwitch();
    if (upEnabled || downEnabled || timerEnabled)
      {
        checkAlarms();
      }
	}

///////////////////////////////////////////////////////////////////////////////
// Alarms
void setTimeAlarm()
  {
    DateTime now = MCP7940.now();
    now = now + TimeSpan(0, AlHr, AlMin, 0);
    MCP7940.setAlarm(0, matchAll, now, true);
  }

void alarm()
  {
    unsigned long currentMillis = millis();

    if (currentMillis - previousMillisAlarm > 500)
      {
        if (buzrState == LOW)
          buzrState = HIGH;
        else
          buzrState = LOW;
        digitalWrite(BZR_PIN, buzrState);
        previousMillisAlarm = currentMillis;
      }
  }

void checkAlarms()
  {
    if (MCP7940.isAlarm(0))
      {
        alarm();
      }
    if (tempC >= upTemp && upEnabled == true)
      {
        alarm();
      }
    if (tempC <= downTemp && downEnabled == true)
      {
        alarm();
      }
  }

void stopAlarm()
  {
    if (MCP7940.getAlarmState(0))
      {
        MCP7940.clearAlarm(0);
        timerEnabled = false;
        digitalWrite(BZR_PIN, LOW);
        tft.fillRect(18, 82, 109, 14, BLACK);
        drawMain();
      }
    if (tempC >= upTemp && upEnabled == true)
      {
        upEnabled = false;
        digitalWrite(BZR_PIN, LOW);
        drawMain();
      }
    if (tempC <= downTemp && downEnabled == true)
      {
        downEnabled = false;
        digitalWrite(BZR_PIN, LOW);
        drawMain();
      }
  }

///////////////////////////////////////////////////////////////////////////////
// Display state machine
void setDisplayMode()
	{
		switch (displayMode)
      {
        case SHOW_MAIN:
          {
						if (mainDisplayed == false)
							{
								drawMain();
								mainDisplayed = true;
							}
            unsigned long currentMillis = millis();
            if (currentMillis - previousMillis >= 1000)
              {
                drawTemp();
    						drawTime();
                showDebug();
                previousMillis = currentMillis;
              }
    				// mode 0
            if (switchMenu == IS_FALLING)
              {
								tft.fillScreen(BLACK);
                displayMode = SHOW_MENU;
								mainDisplayed = false;
								drawMenu();
                break;
              }

            if (switchSel == IS_FALLING)
              {
                stopAlarm();
                break;
              }
          }

        case SHOW_MENU:
          {
    				readNav(2);	//Check for menu selection
						if (highlightToggled == true)
							{
								drawMenu();
								highlightToggled = false;
							}
    				// mode 1
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
								if (highlighted == 2)
									{
										tft.quickFill(BLACK);
										displayMode = SETTINGS_MENU;
										highlighted = 0;
										drawSettingsMenu();
										break;
									}
							}
						break;
          }

				case TEMP_MENU:
					{
						// mode 2
						readNav(3);	// Select up or down temp
						if (highlightToggled)
							{
								drawTempMenu();
								highlightToggled = false;
							}
						if (switchMenu == IS_FALLING)
              {
								tft.fillScreen(BLACK);
                displayMode = SHOW_MENU;
                highlighted = 1;
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
										byte u = 'u';
										enable(u);
										//drawTempMenu();
									}
								if (highlighted == 2)
									{
										displayMode = SET_DOWN_TEMP;
										break;
									}
								if (highlighted == 3)
									{
										byte d = 'd';
										enable(d);
										//drawTempMenu();
									}
							}
						break;
					}

				case SET_UP_TEMP:
					{
						// mode 3
						highlightUpTemp();
						if (switchSel == IS_FALLING)
							{
								displayMode = TEMP_MENU;
								drawTempMenu();
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
                drawMenu();
								break;
							}
						break;
					}

				case SET_DOWN_TEMP:
					{
						// mode 4
						highlightDownTemp();
						if (switchSel == IS_FALLING)
							{
								displayMode = TEMP_MENU;
								drawTempMenu();
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
                drawMenu();
								break;
							}
						break;
					}

				case TIMER_MENU:
					{
						// mode 5
						readNav(3);
						if (highlightToggled)
							{
								drawTimerMenu();
								highlightToggled = false;
							}
						if (switchMenu == IS_FALLING)
              {
								tft.fillScreen(BLACK);
                displayMode = SHOW_MENU;
                highlighted = 0;
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
										//drawTimerMenu();
                    setTimeAlarm();
										//break;
									}
								}
							break;
				 	}

				case SET_HOUR:
					{
						// mode 6
						highlightSetHour();
						if (switchSel == IS_FALLING)
							{
								displayMode = TIMER_MENU;
								drawTimerMenu();
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
                drawMenu();
								break;
							}
						break;
					}

				case SET_MIN:
					{
						// mode 7
						highlightSetMin();
						if (switchSel == IS_FALLING)
							{
								displayMode = TIMER_MENU;
								drawTimerMenu();
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
                drawMenu();
								break;
							}
						break;
					}

				case SETTINGS_MENU:
					{
						//mode 8
						readNav(2);
						if (highlightToggled)
							{
								drawSettingsMenu();
								highlightToggled = false;
							}
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
										displayMode = SET_DEFUP_TEMP;
										break;
									}
								if (highlighted == 1)
									{
										displayMode = SET_DEFDOWN_TEMP;
										break;
									}
							}
              break;
					}

				case SET_DEFUP_TEMP:
					{
						// mode 9
						highlightDefUpTemp();
						if (switchSel == IS_FALLING)
							{
								displayMode = SETTINGS_MENU;
                EEPROM.put(DefUpTemp_addr, defUpTemp);
								drawSettingsMenu();
								break;
							}
						else if (switchUp == IS_CLOSED)
							{
								incrDef();
								break;
							}
						else if (switchDown == IS_CLOSED)
							{
								decrDef();
								break;
							}
						else if (switchMenu == IS_FALLING)
							{
								tft.quickFill(BLACK);
								//menuLevel = 1;
								displayMode = SHOW_MENU;
                EEPROM.put(DefUpTemp_addr, defUpTemp);
                drawMenu();
								break;
							}
						break;
					}

				case SET_DEFDOWN_TEMP:
					{
						// mode 10
						highlightDefDownTemp();
						if (switchSel == IS_FALLING)
							{
								displayMode = SETTINGS_MENU;
                EEPROM.put(DefDownTemp_addr, defDownTemp);
								drawSettingsMenu();
								break;
							}
						else if (switchUp == IS_CLOSED)
							{
								incrDef();
								break;
							}
						else if (switchDown == IS_CLOSED)
							{
								decrDef();
								break;
							}
						else if (switchMenu == IS_FALLING)
							{
								tft.quickFill(BLACK);
								//menuLevel = 1;
								displayMode = SHOW_MENU;
                EEPROM.put(DefDownTemp_addr, defDownTemp);
                drawMenu();
								break;
							}
						break;
					}
      }
	}

///////////////////////////////////////////////////////////////////////////////
// System info
void showDebug()
	{
		tft.setTextColor(BLUE, BLACK);
		tft.setTextSize(1);
		tft.setCursor(2, 110);
		//tft.print("Mode ");
		//tft.print(displayMode);
    tft.print("Batt ");
    float volts = gauge.getVoltage();
    volts = volts/1000;
    tft.print(volts);
    tft.print(" SOC ");
    float soc = gauge.getSOC();
    tft.print(soc);
    tft.print("%");
    tft.setCursor(2, 118);
    tft.print("TTE ");
    float tte = gauge.getTTE();
    tte = tte/3600;
    tft.print(tte);
    //tft.print(" TTF ");
    //float ttf = gauge.getTTF();
    //ttf = ttf/3600;
    //tft.print(ttf);
    float cap = gauge.getCapacity();
    tft.print(" Cap ");
    tft.print(cap);
    //tft.print("mA ");
    //float mA = gauge.getCurrent();
    //tft.print(abs(mA));
    //tft.setCursor(50, 118);
		//tft.print("HL ");
		//tft.print(highlighted);
    //tft.print(" RAM ");
    //tft.print(freeRam());
    //tft.print(" Mode ");
		//tft.print(displayMode);
    tft.setTextSize(2);
	}

int freeRam ()
  {
    extern int __heap_start, *__brkval;
    int v;
    return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
  } // freeRam

////////////////////////////////////////////////////////////////////////////////
// Navigation
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

///////////////////////////////////////////////////////////////////////////////
// temp and Time displays
void drawTemp()
  {
    byte i;
    byte data[9];
    tft.setCursor(16, 17);
    tft.setTextSize(4);
    tft.setTextColor(GREEN, BLACK);
    oneWire.reset();
    oneWire.select(tempSensor);
    oneWire.write(0xBE);
    for ( i = 0; i < 9; i++)  // we need 9 bytes
      {
         data[i] = oneWire.read();
         //tft.print(data[i], HEX);
         //tft.print(" ");
       }
    tempC = ( (data[1] << 8) | data[0] )*0.0625;
    tft.print(tempC);
    oneWire.reset();
    oneWire.select(tempSensor);
    oneWire.write(0x44);
    /*
    unsigned long currentMillis = millis();

    if (currentMillis - previousMillisTemp >= 150)
      {
        tft.setCursor(16, 17);
        tft.setTextSize(4);
    		tft.setTextColor(GREEN, BLACK);
        oneWire.reset();
        oneWire.select(tempSensor);
        oneWire.write(0xBE);
        for ( i = 0; i < 9; i++)  // we need 9 bytes
          {
             data[i] = oneWire.read();
             //tft.print(data[i], HEX);
             //tft.print(" ");
           }
        tempC = ( (data[1] << 8) | data[0] )*0.0625;
        tft.print(tempC);
        oneWire.reset();
        oneWire.select(tempSensor);
        oneWire.write(0x44);
        previousMillisTemp = currentMillis;
      }
      */
  }

void drawTime()
  {
		DateTime now = MCP7940.now();
		sec = now.second();
		min = now.minute();
		hr = now.hour();
    /*
    tft.setCursor(40, 67);
		tft.setTextSize(1);
    sprintf(timeBuff,"%02d:%02d:%02d", hr, min, sec);
		tft.print(timeBuff);
    */
    tft.setTextSize(2);
    if (timerEnabled == true)
      {
        //tft.setCursor(18, 82);
        tft.setCursor(18, 78);
        uint8_t alarmType;
        DateTime alarmTime = MCP7940.getAlarm(0, alarmType);
        int alSec = alarmTime.second();
    		int alMin = alarmTime.minute();
    		int alHr = alarmTime.hour();
        int alSecs = alSec + (alMin * 60) + (alHr * 3600);
        int nowSecs = sec + (min * 60) + (hr * 3600);
        int timeToGo = alSecs - nowSecs;
        int secsToGo = timeToGo % 60;
        int minsToGo = (timeToGo % 3600) / 60;
        int hrsToGo = timeToGo / 3600;
        tft.setTextColor(RED, BLACK);
        // Flash display
        if (timeToGo <=0 && timeToGo % 2 == 0)
          {
            tft.fillRect(18, 78, 109, 14, BLACK);
            return;
          }
    		sprintf(timeBuff,"%02d:%02d:%02d", abs(hrsToGo), abs(minsToGo),
         abs(secsToGo));
        tft.print(timeBuff);
      }
    else
      {
        tft.setCursor(18, 78);
    		tft.setTextColor(DKGREY, BLACK);
        tft.print("00:00:00");
      }
  }

///////////////////////////////////////////////////////////////////////////////
void drawMain()
	{

    tft.drawFastHLine(0, 60, 128, DKBLUE);
		tft.drawFastVLine(80, 0, 60, DKBLUE);
		tft.drawFastHLine(80, 30, 48, DKBLUE);
    tft.drawFastHLine(0, 108, 128, DKBLUE);
		tft.drawRect(0, 0, 128, 128, DKBLUE);

		tft.drawCircle(68, 20, 4, GREEN);
		tft.drawCircle(68, 20, 3, GREEN);
		//Up temp
		tft.setCursor(92, 9);
		tft.setTextSize(2);
    if (upEnabled)
      {
        tft.drawCircle(118, 11, 2, RED);
    		tft.drawCircle(118, 11, 1, RED);
        tft.setTextColor(RED, BLACK);
      }
    else
      {
        tft.drawCircle(118, 11, 2, DKGREY);
    		tft.drawCircle(118, 11, 1, DKGREY);
        tft.setTextColor(DKGREY, BLACK);
      }
		tft.print(upTemp);
		//down Temp
		tft.setCursor(92, 38);
		tft.setTextSize(2);
    if (downEnabled)
      {
        tft.drawCircle(118, 40, 2, RED);
    		tft.drawCircle(118, 40, 1, RED);
        tft.setTextColor(RED, BLACK);
      }
    else
      {
        tft.drawCircle(118, 40, 2, DKGREY);
    		tft.drawCircle(118, 40, 1, DKGREY);
        tft.setTextColor(DKGREY, BLACK);
      }
		tft.print(downTemp);
	}

void drawMenu()
  {
    tft.setCursor(0, 0);
    tft.setTextSize(2);
    if (highlighted == 0)
      {
        tft.setTextColor(WHITE, DKGREEN);
        tft.println("Set Temp");
        tft.setTextColor(GREEN, BLACK);
				tft.setCursor(0, 22);
        tft.println("Set Timer");
				tft.setCursor(0, 44);
        tft.println("Settings");
      }
    else if (highlighted == 1)
      {
        tft.setTextColor(GREEN, BLACK);
        tft.println("Set Temp");
        tft.setTextColor(WHITE, DKGREEN);
				tft.setCursor(0, 22);
        tft.println("Set Timer");
        tft.setTextColor(GREEN, BLACK);
				tft.setCursor(0, 44);
        tft.println("Settings");
      }
    else if (highlighted == 2)
    {
      tft.setTextColor(GREEN, BLACK);
      tft.println("Set Temp");
			tft.setCursor(0, 22);
      tft.println("Set Timer");
      tft.setTextColor(WHITE, DKGREEN);
			tft.setCursor(0, 44);
      tft.println("Settings");
    }
  }

///////////////////////////////////////////////////////////////////////////////
// Temp menu
void drawTempMenu()
	{
		//Up temp
		tft.setCursor(0, 0);
		tft.setTextColor(BLUE, BLACK);
		tft.print(" Set Temp");
		tft.setCursor(0, 22);
		if (highlighted == 0)
			{
				tft.setTextColor(WHITE, DKGREEN);
			}
		else
			{
				tft.setTextColor(GREEN, BLACK);
			}
    tft.print("Maximum");
		tft.setCursor(100, 22);
		tft.setTextColor(RED, BLACK);
		tft.print(upTemp);
		// Enable up temp:
		tft.setCursor(0, 40);	// Prepare to draw enabled switch
		if (highlighted == 1)
			{
				tft.setTextColor(WHITE, DKGREEN);
			}
		else
			{
				tft.setTextColor(GREEN, BLACK);
			}
		tft.print("Enabled");
		tft.drawRoundRect(105, 40, 16, 16, 2, GREEN);
		if (upEnabled == true)
			{
				tft.fillRoundRect(106, 41, 14, 14, 2, RED);
			}

		// Down temp
		tft.setCursor(0, 76);
		if (highlighted == 2)
			{
				tft.setTextColor(WHITE, DKGREEN);
			}
		else
			{
				tft.setTextColor(GREEN, BLACK);
			}
    tft.print("Minimum");
		if (downTemp >= 10)
			{
				tft.setCursor(100, 76);
			}
		else
			{
				tft.setCursor(112, 76);
			}
		tft.setTextColor(RED, BLACK);
    tft.print(downTemp);

		// Enable down temp:
		tft.setCursor(0, 94);	// Prepare to draw enabled switch
		if (highlighted == 3)
			{
				tft.setTextColor(WHITE, DKGREEN);
			}
		else
			{
				tft.setTextColor(GREEN, BLACK);
			}
		tft.print("Enabled");
		tft.drawRoundRect(105, 94, 16, 16, 2, GREEN);
		if (downEnabled == true)
			{
				tft.fillRoundRect(106, 95, 14, 14, 2, RED);
			}
	}

void highlightUpTemp()
	{
		tft.setTextColor(GREEN, BLACK);
		tft.setCursor(0, 22);
		tft.print("Maximum");
    if (upTemp >= 10)
			{
				tft.setCursor(100, 22);
			}
		else
			{
				tft.setCursor(112, 22);
			}
		tft.setTextColor(WHITE, RED);
		tft.print(upTemp);
	}

void highlightDownTemp()
	{
		tft.setTextColor(GREEN, BLACK);
		tft.setCursor(0, 76);
		tft.print("Minimum");
		tft.setTextColor(WHITE, RED);
		if (downTemp >= 10)
			{
				tft.setCursor(100, 76);
			}
		else if (downTemp < 10)
			{
				tft.setCursor(112, 76);
			}
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
		else if (highlighted == 2)
			{
				if (downTemp >= 9)
					{
            tft.setCursor(100, 76);
						downTemp += 5;
						//tft.print(downTemp);
					}
				else if (downTemp < 9)
					{
						tft.fillRect(100, 76, 12, 16, BLACK);
						tft.setCursor(112, 76);
						downTemp += 5;
						//tft.print(downTemp);
					}
        tft.print(downTemp);
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
		else if (highlighted == 2)
		{
			if (downTemp > 10 )
				{
					downTemp -= 5;
					tft.setCursor(100, 76);
					//tft.print(downTemp);
				}
			else if (downTemp >= 1)
				{
					tft.fillRect(100, 76, 12, 16, BLACK);
					tft.setCursor(112, 76);
					downTemp -= 5;
					//tft.print(downTemp);
				}
      tft.print(downTemp);
		}
	}

///////////////////////////////////////////////////////////////////////////////
// Timer menu
void drawTimerMenu()
	{
		tft.setCursor(0, 0);
		tft.setTextColor(BLUE, BLACK);
		tft.println(" Set Timer");
		tft.setCursor(0, 22);	// Prepare to draw hours
		if (highlighted == 0)
			{
				tft.setTextColor(WHITE, DKGREEN);
			}
		else
			{
				tft.setTextColor(GREEN, BLACK);
			}
		tft.print("Hours");
		tft.setCursor(100, 22);
		tft.setTextColor(RED, BLACK);
		if (AlHr < 10)
			{
				tft.print(0);
			}
    tft.print(AlHr);

		tft.setCursor(0, 40);	// Prepare to draw minutes
		if (highlighted == 1)
			{
				tft.setTextColor(WHITE, DKGREEN);
			}
		else
			{
				tft.setTextColor(GREEN, BLACK);
			}
		tft.print("Mins");
		tft.setCursor(100, 40);
		tft.setTextColor(RED, BLACK);
		if (AlMin < 10)
			{
				tft.print(0);
			}
    tft.print(AlMin);

		tft.setCursor(0, 58);	// Prepare to draw enabled switch
		if (highlighted == 2)
			{
				tft.setTextColor(WHITE, DKGREEN);
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
		tft.setTextColor(WHITE, RED);
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
		tft.setTextColor(WHITE, RED);
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
		if (highlighted == 0)	//decrement SET_HOUR
			{
				//tft.setTextColor(GREEN, RED);
				if (AlHr >= 1)
					{
						tft.setCursor(100, 22);
						AlHr -= 1;
						if (AlHr < 10)
							{
								tft.print(0);
							}
				    tft.print(AlHr);
					}
			}
		else if (highlighted == 1)
			{
				//tft.setTextColor(GREEN, RED);
				if (AlMin >= 1)
					{
						tft.setCursor(100, 40);
						AlMin -= 1;
						if (AlMin < 10)
							{
								tft.print(0);
							}
						tft.print(AlMin);
					}
			}
	}

///////////////////////////////////////////////////////////////////////////////
// settings menu
void drawSettingsMenu()
	{
		tft.setCursor(0, 0);
		tft.setTextColor(BLUE, BLACK);
		tft.print(" Settings");
		tft.setCursor(0, 22);
		if (highlighted == 0)
			{
				tft.setTextColor(WHITE, DKGREEN);
			}
		else
			{
				tft.setTextColor(GREEN, BLACK);
			}
    tft.print("Max Temp");
    if (defUpTemp >= 10)
			{
				tft.setCursor(100, 22);
			}
		else
			{
				tft.setCursor(112, 22);
			}
		tft.setTextColor(RED, BLACK);
		tft.print(defUpTemp);

		tft.setCursor(0, 40);
		if (highlighted == 1)
			{
				tft.setTextColor(WHITE, DKGREEN);
			}
		else
			{
				tft.setTextColor(GREEN, BLACK);
			}
		tft.print("Min Temp");
    if (defDownTemp >= 10)
			{
				tft.setCursor(100, 40);
			}
		else
			{
				tft.setCursor(112, 40);
			}
		tft.setTextColor(RED, BLACK);
		tft.print(defDownTemp);
	}

void highlightDefUpTemp()
	{
		tft.setTextColor(GREEN, BLACK);
		tft.setCursor(0, 22);
		tft.print("Max Temp");
    if (defUpTemp >= 10)
			{
				tft.setCursor(100, 22);
			}
		else
			{
				tft.setCursor(112, 22);
			}
		tft.setTextColor(WHITE, RED);
		tft.print(defUpTemp);
	}

void highlightDefDownTemp()
	{
		tft.setTextColor(GREEN, BLACK);
		tft.setCursor(0, 40);
		tft.print("Min Temp");
		tft.setTextColor(WHITE, RED);
		if (defDownTemp >= 10)
			{
				tft.setCursor(100, 40);
			}
		else if (defDownTemp < 10)
			{
				tft.setCursor(112, 40);
			}
		tft.print(defDownTemp);
	}

void incrDef()
	{
		if (highlighted == 0)
		{
			if (defUpTemp < 99)
				{
          if (defUpTemp >= 10)
      			{
      				tft.setCursor(100, 22);
      			}
      		else
      			{
      				tft.setCursor(112, 22);
      			}
					defUpTemp += 1;
					tft.print(defUpTemp);
				}
		}

		else if (highlighted == 1)
			{
				tft.setCursor(100, 40);
				if (defDownTemp >= 9)
					{
						defDownTemp += 1;
						tft.print(defDownTemp);
					}
				else if (defDownTemp < 9)
					{
						tft.fillRect(100, 40, 12, 16, BLACK);
						tft.setCursor(112, 40);
						defDownTemp += 1;
						tft.print(defDownTemp);
					}
			}
	}

void decrDef()
	{
		if (highlighted == 0)
		{
			if (defUpTemp  > 1)
				{
          if (defUpTemp >= 10)
      			{
      				tft.setCursor(100, 22);
      			}
      		else
      			{
      				tft.setCursor(112, 22);
      			}
					defUpTemp -= 1;
					tft.print(defUpTemp);
				}
		}

		else if (highlighted == 1)
		{
			if (defDownTemp > 10 )
				{
					defDownTemp -= 5;
					tft.setCursor(100, 40);
					tft.print(defDownTemp);
				}
			else if (defDownTemp >= 1)
				{
					tft.fillRect(100, 40, 12, 16, BLACK);
					tft.setCursor(112, 40);
					defDownTemp -= 5;
					tft.print(defDownTemp);
				}
		}
	}

///////////////////////////////////////////////////////////////////////////////
// Enable
void enable(byte mode)
	{
		if (mode == 'u')	// temp up
			{
				if (upEnabled == true)
					{
						//tft.drawRoundRect(105, 40, 16, 16, 2, GREEN);
						tft.fillRoundRect(106, 59, 14, 14, 2, BLACK);
						upEnabled = false;
					}
				else
					{
						//tft.drawRoundRect(105, 40, 16, 16, 2, GREEN);
						tft.fillRoundRect(106, 41, 14, 14, 2, RED);
						upEnabled = true;
					}
			}

		if (mode == 'd')	// temp down
			{
				if (downEnabled == true)
					{
						//tft.drawRoundRect(105, 94, 16, 16, 2, GREEN);
						tft.fillRoundRect(106, 95, 14, 14, 2, BLACK);
						downEnabled = false;
					}
				else
					{
						//tft.drawRoundRect(105, 94, 16, 16, 2, GREEN);
						tft.fillRoundRect(106, 95, 14, 14, 2, RED);
						downEnabled = true;
					}
			}

		if (mode == 't')	// timer
			{
				if (timerEnabled == true)
					{
						//tft.drawRoundRect(105, 58, 16, 16, 2, GREEN);
						tft.fillRoundRect(106, 59, 14, 14, 2, BLACK);
						timerEnabled = false;
					}
				else
					{
						//tft.drawRoundRect(105, 58, 16, 16, 2, GREEN);
						tft.fillRoundRect(106, 59, 14, 14, 2, RED);
						timerEnabled = true;
					}
		   }
	}

///////////////////////////////////////////////////////////////////////////////
// Switches
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
