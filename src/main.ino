#include <Arduino.h>

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
#include <Adafruit_SSD1351.h> // OLED
#include <SPI.h>
#include <MCP7940.h>  // RTC
#include <max1720x.h> // Batt Gauge
#include <OneWire.h>
#include <EEPROM.h>
#include <Display.h>
#include <MainMenu.h>
#include <TimeMenu.h>
#include <TempMenu.h>
#include <SettingsMenu.h>

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
 SET_DEFDOWN_TEMP, SET_RTC_MENU, SET_RTC_HR, SET_RTC_MIN};
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
byte buzrState = LOW;

bool mainDisplayed = false;
bool upEnabled = false;
bool downEnabled = false;
bool timerEnabled = false;
bool milliSignal = false;

byte AlMin = 0;
byte AlHr = 0;

Display display;
MainMenu mainMenu;
TimeMenu timeMenu;
TempMenu tempMenu;
SettingsMenu settingsMenu;

/*
 * Sections of main.ino:
 * Switches S/M   line112   Define SwitchStates for the four control buttons
 * Navigation     line236   Control menu highlighting
 * Alarms         line285   All alarm functions including colouring checkboxes
 * Setup          line390
 * loop           line443
 * Display S/M    line474   Define DisplayModes and act on them
 */

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
// Alarms
void setTimeAlarm()
  {
    DateTime now = MCP7940.now();
    now = now + TimeSpan(0, AlHr, AlMin, 0);
    MCP7940.setAlarm(0, 7, now, true);
  }   // setTimeAlarm()

void alarm()
  {
    if (milliSignal == true)
      {
        tone(BZR_PIN, 2030, 500);
      }
    }   // alarm()

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
        noTone(BZR_PIN);
        tft.fillRect(18, 67, 109, 25, BLACK);
        display.drawMainDisplay(tft);
      }
    if (tempC >= upTemp && upEnabled == true)
      {
        upEnabled = false;
        noTone(BZR_PIN);
        display.drawMainDisplay(tft);
      }
    if (tempC <= downTemp && downEnabled == true)
      {
        downEnabled = false;
        noTone(BZR_PIN);
        display.drawMainDisplay(tft);
      }
  }

// Enable alarms
void enable(byte mode)
  {
    if (mode == 'u')	// temp up
      {
        if (upEnabled == true)
          {
            tft.fillRoundRect(106, 59, 14, 14, 2, BLACK);
            upEnabled = false;
          }
        else
          {
            tft.fillRoundRect(106, 41, 14, 14, 2, RED);
            upEnabled = true;
          }
      }

    if (mode == 'd')	// temp down
      {
        if (downEnabled == true)
          {
            tft.fillRoundRect(106, 95, 14, 14, 2, BLACK);
            downEnabled = false;
          }
        else
          {
            tft.fillRoundRect(106, 95, 14, 14, 2, RED);
            downEnabled = true;
          }
      }

    if (mode == 't')	// timer
      {
        if (timerEnabled == true)
          {
            tft.fillRoundRect(106, 59, 14, 14, 2, BLACK);
            timerEnabled = false;
          }
        else
          {
            tft.fillRoundRect(106, 59, 14, 14, 2, RED);
            timerEnabled = true;
            setTimeAlarm();
          }
       }
  }

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void setup()
  {
    // TODO INPUT_PULLUP not set on ICSP lines
    pinMode(MENU_PIN, INPUT_PULLUP);
  	pinMode(UP_PIN, INPUT_PULLUP);
  	pinMode(DOWN_PIN, INPUT_PULLUP);
  	pinMode(SEL_PIN, INPUT_PULLUP);
    pinMode(BZR_PIN, OUTPUT);
    digitalWrite(BZR_PIN, LOW);

    tft.begin();
    tft.setRotation(2);
    tft.setTextColor(CYAN);
  	while (!MCP7940.begin())
  		{
        // Initialize RTC communications
  			tft.fillScreen(BLACK);
  			tft.setCursor(5, 5);
  			tft.println(F("Unable to find RTC"));
  			delay(2000);
  	 	}
   while (!MCP7940.deviceStatus()) // Turn oscillator on if necessary
    {
  		tft.fillScreen(BLACK);
  		tft.setCursor(5, 5);
      tft.println(F("Oscillator is off, turning it on."));
      bool deviceStatus = MCP7940.deviceStart(); // Start oscillator and return state
      if (!deviceStatus) // If it didn't start
      {
  			tft.fillScreen(BLACK);
  	  	tft.setCursor(5, 5);
        tft.println(F("Oscillator did not start, trying again."));
        delay(1000);
      }
    }
    tft.fillScreen(BLACK);
    tft.setCursor(5, 5);
    tft.println(F("Oscillator started..."));

    tft.fillScreen(BLACK);
    tft.setCursor(0, 0);
    tft.setTextSize(2);
    EEPROM.get(DefUpTemp_addr, defUpTemp);
    EEPROM.get(DefDownTemp_addr, defDownTemp);
    upTemp = defUpTemp;
    downTemp = defDownTemp;
    oneWire.select(tempSensor);
    oneWire.write(0x1F);  // set 9-bit resolution
    gauge.reset();
  } // setup()

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void loop()
  {
    milliTimer();
    setDisplayMode();
		menuSwitch();
		selectSwitch();
    upSwitch();
    downSwitch();
    if (upEnabled || downEnabled || timerEnabled)
      {
        checkAlarms();
      }
	}  // loop()
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Milli Timer
void milliTimer()
  {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= 500)
      {
        milliSignal = true;
      }
    else
      {
        milliSignal = false;
      }
    previousMillis = currentMillis;
  } // milliTimer()

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
								display.drawMainDisplay(tft);
								mainDisplayed = true;
							}
            if (milliSignal == false)
              {
                display.drawTemp(tft, oneWire, tempSensor, tempC);
    						display.drawTime(tft, MCP7940, timeBuff, timerEnabled);
                display.drawInfo(tft, gauge);
              }
    				// mode 0
            if (switchMenu == IS_FALLING)
              {
								tft.fillScreen(BLACK);
                displayMode = SHOW_MENU;
								mainDisplayed = false;
								mainMenu.drawMenu(tft);
                break;
              }

            if (switchSel == IS_FALLING)
              {
                stopAlarm();
                break;
              }
          } // case SHOW_MAIN:

        case SHOW_MENU:
          {
    				readNav(2);	//Check for menu selection
						if (highlightToggled == true)
							{
								mainMenu.drawMenu(tft);
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
                    tempMenu.setTemps(tft, upTemp, downTemp, upEnabled,
                       downEnabled);
										break;
									}
								if (highlighted == 1)
									{
										tft.quickFill(BLACK);
										displayMode = TIMER_MENU;
										highlighted = 0;
										timeMenu.setTimer(tft, AlHr, AlMin, timerEnabled);
										break;
									}
								if (highlighted == 2)
									{
										tft.quickFill(BLACK);
										displayMode = SETTINGS_MENU;
										highlighted = 0;
										settingsMenu.settings(tft, defUpTemp, defDownTemp);
										break;
									}
							}
						break;
          } // case SHOW_MENU:

				case TEMP_MENU:
					{
						// mode 2
						readNav(3);	// Select up or down temp
						if (highlightToggled)
							{
								tempMenu.setTemps(tft, upTemp, downTemp, upEnabled,
                   downEnabled);
								highlightToggled = false;
							}
						if (switchMenu == IS_FALLING)
              {
								tft.fillScreen(BLACK);
                displayMode = SHOW_MENU;
                highlighted = 1;
								mainMenu.drawMenu(tft);
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
									}
							}
						break;
					}    // case TEMP_MENU:

				case SET_UP_TEMP:
					{
						// mode 3
						tempMenu.highlightUpTemp(tft);
						if (switchSel == IS_FALLING)
							{
								displayMode = TEMP_MENU;
								tempMenu.drawTempMenu(tft);
								break;
							}
						else if (switchUp == IS_CLOSED)
							{
								upTemp = tempMenu.incrTemp(tft);
								break;
							}
						else if (switchDown == IS_CLOSED)
							{
								upTemp = tempMenu.decrTemp(tft);
								break;
							}
						else if (switchMenu == IS_FALLING)
							{
								tft.quickFill(BLACK);
								//menuLevel = 1;
								displayMode = SHOW_MENU;
                mainMenu.drawMenu(tft);
								break;
							}
						break;
					}    // case SET_UP_TEMP:

				case SET_DOWN_TEMP:
					{
						// mode 4
						tempMenu.highlightDownTemp(tft);
						if (switchSel == IS_FALLING)
							{
								displayMode = TEMP_MENU;
								tempMenu.drawTempMenu(tft);
								break;
							}
						else if (switchUp == IS_CLOSED)
							{
								downTemp = tempMenu.incrTemp(tft);
								break;
							}
						else if (switchDown == IS_CLOSED)
							{
								downTemp = tempMenu.decrTemp(tft);
								break;
							}
						else if (switchMenu == IS_FALLING)
							{
								tft.quickFill(BLACK);
								displayMode = SHOW_MENU;
                mainMenu.drawMenu(tft);
								break;
							}
						break;
					}    // case SET_DOWN_TEMP:

				case TIMER_MENU:
					{
						// mode 5
						readNav(3);
						if (highlightToggled)
							{
                timeMenu.setTimer(tft, AlHr, AlMin, timerEnabled);
								highlightToggled = false;
							}
						if (switchMenu == IS_FALLING)
              {
								tft.fillScreen(BLACK);
                displayMode = SHOW_MENU;
                highlighted = 0;
								mainMenu.drawMenu(tft);
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
										break;
									}
								}
							break;
				 	}     // case TIMER_MENU:

				case SET_HOUR:
					{
						// mode 6
						timeMenu.highlightSetHour(tft);
						if (switchSel == IS_FALLING)
							{
								displayMode = TIMER_MENU;
                timeMenu.setTimer(tft, AlHr, AlMin, timerEnabled);
								break;
							}
						else if (switchUp == IS_CLOSED)
							{
								AlHr = timeMenu.incrTime(tft);
								break;
							}
						else if (switchDown == IS_CLOSED)
							{
								AlHr = timeMenu.decrTime(tft);
								break;
							}
						else if (switchMenu == IS_FALLING)
							{
								tft.quickFill(BLACK);
								//menuLevel = 1;
								displayMode = SHOW_MENU;
                mainMenu.drawMenu(tft);
								break;
							}
						break;
					}    // case SET_HOUR:

				case SET_MIN:
					{
						// mode 7
						timeMenu.highlightSetMin(tft);
						if (switchSel == IS_FALLING)
							{
								displayMode = TIMER_MENU;
                timeMenu.setTimer(tft, AlHr, AlMin, timerEnabled);
								break;
							}
						else if (switchUp == IS_CLOSED)
							{
								AlMin = timeMenu.incrTime(tft);
								break;
							}
						else if (switchDown == IS_CLOSED)
							{
								AlMin = timeMenu.decrTime(tft);
								break;
							}
						else if (switchMenu == IS_FALLING)
							{
								tft.quickFill(BLACK);
								//menuLevel = 1;
								displayMode = SHOW_MENU;
                mainMenu.drawMenu(tft);
								break;
							}
						break;
					}    // case SET_MIN:

				case SETTINGS_MENU:
					{
						//mode 8
						readNav(2);
						if (highlightToggled)
							{
								settingsMenu.drawSettingsMenu(tft);
								highlightToggled = false;
							}
						if (switchMenu == IS_FALLING)
              {
								tft.fillScreen(BLACK);
                displayMode = SHOW_MENU;
								mainMenu.drawMenu(tft);
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
                if (highlighted == 2)
                  {
                    tft.fillScreen(BLACK);
                    displayMode = SET_RTC_MENU;
                    highlighted = 0;
                    timeMenu.setRTC(tft, MCP7940);
                    break;
                  }
							}
              break;
					}      // case SETTINGS_MENU:

				case SET_DEFUP_TEMP:
					{
						// mode 9
						settingsMenu.highlightDefUpTemp(tft);
						if (switchSel == IS_FALLING)
							{
								displayMode = SETTINGS_MENU;
                EEPROM.put(DefUpTemp_addr, defUpTemp);
								settingsMenu.drawSettingsMenu(tft);
								break;
							}
						else if (switchUp == IS_CLOSED)
							{
								defUpTemp = settingsMenu.incrDef(tft);
								break;
							}
						else if (switchDown == IS_CLOSED)
							{
								defUpTemp = settingsMenu.decrDef(tft);
								break;
							}
						else if (switchMenu == IS_FALLING)
							{
								tft.quickFill(BLACK);
								//menuLevel = 1;
								displayMode = SHOW_MENU;
                EEPROM.put(DefUpTemp_addr, defUpTemp);
                mainMenu.drawMenu(tft);
								break;
							}
						break;
					}    // case SET_DEFUP_TEMP:

				case SET_DEFDOWN_TEMP:
					{
						// mode 10
						settingsMenu.highlightDefDownTemp(tft);
						if (switchSel == IS_FALLING)
							{
								displayMode = SETTINGS_MENU;
                EEPROM.put(DefDownTemp_addr, defDownTemp);
								settingsMenu.drawSettingsMenu(tft);
								break;
							}
						else if (switchUp == IS_CLOSED)
							{
								defDownTemp = settingsMenu.incrDef(tft);
								break;
							}
						else if (switchDown == IS_CLOSED)
							{
								defDownTemp = settingsMenu.decrDef(tft);
								break;
							}
						else if (switchMenu == IS_FALLING)
							{
								tft.quickFill(BLACK);
								//menuLevel = 1;
								displayMode = SHOW_MENU;
                EEPROM.put(DefDownTemp_addr, defDownTemp);
                mainMenu.drawMenu(tft);
								break;
							}
						break;
					}    // case SET_DEFDOWN_TEMP

        case SET_RTC_MENU:
          {
            readNav(2);
            if (highlightToggled)
							{
                timeMenu.setRTC(tft, MCP7940);
								highlightToggled = false;
							} // if (highlightToggled)
						if (switchMenu == IS_FALLING)
              {
								tft.fillScreen(BLACK);
                displayMode = SHOW_MENU;
                highlighted = 0;
								mainMenu.drawMenu(tft);
								break;
              } // if (switchMenu == IS_FALLING)
						if (switchSel == IS_FALLING)
							{
								if (highlighted == 0)
									{
										displayMode = SET_RTC_HR;
										break;
									}
								if (highlighted == 1)
									{
										displayMode = SET_RTC_MIN;
										break;
									}
								if (highlighted == 2)
									{
										timeMenu.adjustRTC(MCP7940);
                    timeMenu.RTC_set = false;
                    tft.fillScreen(BLACK);
                    displayMode = SHOW_MENU;
                    highlighted = 0;
    								mainMenu.drawMenu(tft);
										break;
									}
								}  // if (switchSel == IS_FALLING)
							break;
          } // case SET_RTC_MENU

        case SET_RTC_HR:
					{
						timeMenu.highlightSetHour(tft);
						if (switchSel == IS_FALLING)
							{
								displayMode = SET_RTC_MENU;
                char title[10] = "  Set RTC";
								timeMenu.drawTimerMenu(tft, title);
								break;
							}
						else if (switchUp == IS_CLOSED)
							{
								timeMenu.incrTime(tft);
								break;
							}
						else if (switchDown == IS_CLOSED)
							{
								timeMenu.decrTime(tft);
								break;
							}
						else if (switchMenu == IS_FALLING)
							{
								tft.quickFill(BLACK);
								//menuLevel = 1;
								displayMode = SHOW_MENU;
                mainMenu.drawMenu(tft);
								break;
							}
						break;
					}    // case SET_RTC_HR

				case SET_RTC_MIN:
					{
						timeMenu.highlightSetMin(tft);
						if (switchSel == IS_FALLING)
							{
								displayMode = SET_RTC_MENU;
                char title[10] = "  Set RTC";
								timeMenu.drawTimerMenu(tft, title);
								break;
							}
						else if (switchUp == IS_CLOSED)
							{
								timeMenu.incrTime(tft);
								break;
							}
						else if (switchDown == IS_CLOSED)
							{
								timeMenu.decrTime(tft);
								break;
							}
						else if (switchMenu == IS_FALLING)
							{
								tft.quickFill(BLACK);
								//menuLevel = 1;
								displayMode = SHOW_MENU;
                mainMenu.drawMenu(tft);
								break;
							}
						break;
					}   // case SET_RTC_MIN
      }  // switch
	}  // setDisplayMode
