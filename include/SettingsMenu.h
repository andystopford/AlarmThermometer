/*
 * Draws settings menus
 */

class SettingsMenu
  {
  public:
    byte defUpTemp;
    byte defDownTemp;

    SettingsMenu()
      {}

    void settings(Adafruit_SSD1351 tft, byte &defUpT, byte &defDownT)
      {
        defUpTemp = defUpT;
        defDownTemp = defDownT;
        drawSettingsMenu(tft);
      } // settings

    void drawSettingsMenu(Adafruit_SSD1351 tft)
    	{
        extern byte highlighted;
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
        // RTC set
        tft.setCursor(0, 58);
        if (highlighted == 2)
    			{
    				tft.setTextColor(WHITE, DKGREEN);
    			}
    		else
    			{
    				tft.setTextColor(GREEN, BLACK);
    			}
        tft.print("Set RTC");
    	}  // drawSettingsMenu()

    void highlightDefUpTemp(Adafruit_SSD1351 tft)
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
    	}  // highlightDefUpTemp()

    void highlightDefDownTemp(Adafruit_SSD1351 tft)
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
    	}  // highlightDefDownTemp()

    byte incrDef(Adafruit_SSD1351 tft)
    	{
        extern byte highlighted;
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
              return defUpTemp;
    				}
    		}   // if (highlighted == 0)

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
            return defDownTemp;
    			}  // else if (highlighted == 1)
          return 0;
    	}  // incrDef()

    byte decrDef(Adafruit_SSD1351 tft)
    	{
        extern byte highlighted;
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
              return defUpTemp;
    				}
    		}   // if (highlighted == 0)

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
          return defDownTemp;
    		} // else if (highlighted == 1)
        return 0;
    	}  // decrDef()
  };
