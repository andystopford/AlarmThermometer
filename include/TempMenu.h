/*
 *  Draws temperature-related menus
 */

class TempMenu
  {
  public:
    byte upT;
    byte downT;
    bool upE = false;
    bool downE = false;

    TempMenu()
      {}

    void setTemps(Adafruit_SSD1351 tft, byte &upTemp, byte &downTemp, bool upEnabled, bool downEnabled)
      {
        upT = upTemp;
        downT = downTemp;
        upE = upEnabled;
        downE = downEnabled;
        drawTempMenu(tft);
      } // setTemps()

    void drawTempMenu(Adafruit_SSD1351 tft)
    	{
        extern byte highlighted;
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
    		tft.print(upT);
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
    		if (upE == true)
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
    		if (downT >= 10)
    			{
    				tft.setCursor(100, 76);
    			}
    		else
    			{
    				tft.setCursor(112, 76);
    			}
    		tft.setTextColor(RED, BLACK);
        tft.print(downT);

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
    		if (downE == true)
    			{
    				tft.fillRoundRect(106, 95, 14, 14, 2, RED);
    			}
    	}  // drawTempMenu

    void highlightUpTemp(Adafruit_SSD1351 tft)
    	{
    		tft.setTextColor(GREEN, BLACK);
    		tft.setCursor(0, 22);
    		tft.print("Maximum");
        if (upT >= 10)
    			{
    				tft.setCursor(100, 22);
    			}
    		else
    			{
    				tft.setCursor(112, 22);
    			}
    		tft.setTextColor(WHITE, RED);
    		tft.print(upT);
    	}  // highlightUpTemp

    void highlightDownTemp(Adafruit_SSD1351 tft)
    	{
    		tft.setTextColor(GREEN, BLACK);
    		tft.setCursor(0, 76);
    		tft.print("Minimum");
    		tft.setTextColor(WHITE, RED);
    		if (downT >= 10)
    			{
    				tft.setCursor(100, 76);
    			}
    		else if (downT < 10)
    			{
    				tft.setCursor(112, 76);
    			}
    		tft.print(downT);
    	}  // highlightDownTemp

    byte incrTemp(Adafruit_SSD1351 tft)
    	{
        extern byte highlighted;
    		if (highlighted == 0)
    		{
    			if (upT < 99)
    				{
    					tft.setCursor(100, 22);
    					upT += 1;
    					tft.print(upT);
              return upT;
    				}
    		}
    		else if (highlighted == 2)
    			{
    				if (downT >= 9)
    					{
                tft.setCursor(100, 76);
    						downT += 5;
    					}
    				else if (downT < 9)
    					{
    						tft.fillRect(100, 76, 12, 16, BLACK);
    						tft.setCursor(112, 76);
    						downT += 5;
    					}
            tft.print(downT);
            return downT;
    			}
          return 0;
    	}  // incrTemp

    byte decrTemp(Adafruit_SSD1351 tft)
    	{
        extern byte highlighted;
    		if (highlighted == 0)
    		{
    			if (upT  > 1)
    				{
    					tft.setCursor(100, 22);
    					upT -= 1;
    					tft.print(upT);
              return upT;
    				}
    		}
    		else if (highlighted == 2)
    		{
    			if (downT > 10 )
    				{
    					downT -= 5;
    					tft.setCursor(100, 76);
    				}
    			else if (downT >= 1)
    				{
    					tft.fillRect(100, 76, 12, 16, BLACK);
    					tft.setCursor(112, 76);
    					downT -= 5;
    				}
          tft.print(downT);
          return downT;
    		}
        return 0;
    	}  // decrTemp
  };
