
class TimeMenu
  {
    /*
     *  Draws time-related menus, including setting alarm time and adjusting
     *  the RTC
     */
  public:
    uint8_t hr;
    uint8_t min;
    bool RTC_set = false;
    TimeMenu()
      {  }

    void setTimer(Adafruit_SSD1351 tft, byte &AlHr, byte &AlMin,
      bool timerEnabled)
      {
        /*
         * Timer setting mode
         */
        hr = AlHr;
        min = AlMin;
        char title[11] = " Set Timer";
        byte ypos = 58; // Y posn to draw enabled switch
        drawTimerMenu(tft, title);
        drawSwitch(tft, ypos, true, timerEnabled);
      } // setTimer

    void setRTC(Adafruit_SSD1351 tft, MCP7940_Class MCP7940)
      {
        /*
         *  Enter the  RTC setting mode
         */
        if (RTC_set == false)
          {
            DateTime now = MCP7940.now();
            hr = now.hour();
        		min = now.minute();
            RTC_set = true;
          } // if (RTC_set == false)
        char title[10] = "  Set RTC";
        byte ypos = 58; // Y posn to draw enabled switch
        drawTimerMenu(tft, title);
        drawSwitch(tft, ypos, false, false);
      } // setRTC

    void adjustRTC(MCP7940_Class MCP7940)
      {
        /*
         * Adjust the RTC to the entered hour and minute values (secs = 0)
         */
        DateTime now = MCP7940.now();
        uint16_t yr = now.year();
        uint8_t mnth = now.month();
        uint8_t day = now.day();
        uint8_t sec = 0;
        MCP7940.adjust(DateTime(yr, mnth, day, hr, min, sec));
      }

    void drawTimerMenu(Adafruit_SSD1351 tft, char *title)
      {
        /*
         * Draws a menu with hours and minutes
         */
        extern byte highlighted;
        tft.setCursor(0, 0);
    		tft.setTextColor(BLUE, BLACK);
    		tft.println(title);
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
    		if (hr < 10)
    			{
    				tft.print(0);
    			}
        tft.print(hr);

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
    		if (min < 10)
    			{
    				tft.print(0);
    			}
        tft.print(min);
      } // drawTimerMenu

    void drawSwitch(Adafruit_SSD1351 tft, byte ypos, bool chkbox, bool enabled)
      {
        /*
         * Draws a label and (optional) checkbox
         */
        extern byte highlighted;
        tft.setCursor(0, 58);	// Prepare to draw enabled switch
    		if (highlighted == 2)
    			{
    				tft.setTextColor(WHITE, DKGREEN);
    			}  // if (highlighted == 2)
    		else
    			{
    				tft.setTextColor(GREEN, BLACK);
    			}  // else
        if (chkbox)
          {
            tft.print("Enabled");
        		tft.drawRoundRect(105, 58, 16, 16, 2, GREEN);
        		if (enabled)
        			{
        				tft.fillRoundRect(106, 59, 14, 14, 2, RED);
        			}  // if (enabled == true)
          } // if (chkbox)
        else
          {
            tft.print("Set Time");
          } // else
      } // drawSwitch

    void highlightSetHour(Adafruit_SSD1351 tft)
    	{
    		tft.setTextColor(GREEN, BLACK);
    		tft.setCursor(0, 22);
    		tft.print("Hours");
    		tft.setTextColor(WHITE, RED);
    		tft.setCursor(100, 22);
    		if (hr >= 24)
    			{
    				hr = 0;
    			}
    		if (hr < 10)
    			{
    				tft.print(0);
    			}
    		tft.print(hr);
    	}  // highlightSetHour

    void highlightSetMin(Adafruit_SSD1351 tft)
    	{
    		tft.setTextColor(GREEN, BLACK);
    		tft.setCursor(0, 40);
    		tft.print("Mins");
    		tft.setTextColor(WHITE, RED);
    		tft.setCursor(100, 40);
    		if (min > 59)
    			{
    				min = 0;
    			}
    		if (min < 10)
    			{
    				tft.print(0);
    			}
    		tft.print(min);
    	}  // highlightSetMin

    byte incrTime(Adafruit_SSD1351 tft)
    	{
        extern byte highlighted;
    		if (highlighted == 0)	//increment SET_HOUR
    			{
    				tft.setCursor(100, 22);
    				hr += 1;
    				if (hr >= 24)
    					{
    						hr = 0;
    					}
    				if (hr < 10)
    					{
    						tft.print(0);
    					}
    		    tft.print(hr);
            return hr;
    			}  // if (highlighted == 0)
    		else if (highlighted == 1)
    			{
    				tft.setCursor(100, 40);
    				min += 1;
    				if (min > 59)
    					{
    						min = 0;
    					}
    				if (min < 10)
    					{
    						tft.print(0);
    					}
    				tft.print(min);
            return min;
    			}  // else if (highlighted == 1)
        return 0;
    	}  //incrTime

    byte decrTime(Adafruit_SSD1351 tft)
    	{
        extern byte highlighted;
    		if (highlighted == 0)	//decrement SET_HOUR
    			{
    				if (hr >= 1)
    					{
    						tft.setCursor(100, 22);
    						hr -= 1;
    						if (hr < 10)
    							{
    								tft.print(0);
    							}
    				    tft.print(hr);
                return hr;
    					}
    			}    // if (highlighted == 0)
    		else if (highlighted == 1)
    			{
    				if (min >= 1)
    					{
    						tft.setCursor(100, 40);
    						min -= 1;
    						if (min < 10)
    							{
    								tft.print(0);
    							}
    						tft.print(min);
                return min;
    					}
    			}    // else if (highlighted == 1)
        return 0;
      } // decrTime
  };
