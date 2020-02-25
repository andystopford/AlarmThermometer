/*
 * Draw main display
 */

class Display
  {
  public:

    void drawMainDisplay(Adafruit_SSD1351 tft)
    	{
        extern bool upEnabled;
        extern bool downEnabled;
        extern byte upTemp;
        extern byte downTemp;
        tft.drawFastHLine(0, 60, 128, DKBLUE);
    		tft.drawFastVLine(80, 0, 60, DKBLUE);
    		tft.drawFastHLine(80, 30, 48, DKBLUE);
        tft.drawFastHLine(0, 108, 128, DKBLUE);
    		tft.drawRoundRect(0, 0, 128, 128, 10, DKBLUE);

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
    	}  //drawMain()

    void drawTemp(Adafruit_SSD1351 tft, OneWire oneWire, byte
       tempSensor[], byte &tempC)
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
      } // void drawTemp()

    void drawTime(Adafruit_SSD1351 tft, MCP7940_Class MCP7940,
      char timeBuff[], bool timerEnabled)
      {
    		DateTime now = MCP7940.now();
    		byte sec = now.second();
    		byte min = now.minute();
    		byte hr = now.hour();
        tft.setTextSize(2);
        if (timerEnabled == true)
          {
            uint8_t alarmType;
            DateTime alarmTime = MCP7940.getAlarm(0, alarmType);
            int alSec = alarmTime.second();
        		int alMin = alarmTime.minute();
        		int alHr = alarmTime.hour();
            // Draw finish time
            tft.setCursor(43, 67);
        		tft.setTextSize(1);
            sprintf(timeBuff,"%02d:%02d:%02d", alHr, alMin, alSec);
        		tft.print(timeBuff);
            // Calculate time to go
            int alSecs = alSec + (alMin * 60) + (alHr * 3600);
            int nowSecs = sec + (min * 60) + (hr * 3600);
            int timeToGo = alSecs - nowSecs;
            int secsToGo = timeToGo % 60;
            int minsToGo = (timeToGo % 3600) / 60;
            int hrsToGo = timeToGo / 3600;
            tft.setCursor(18, 78);
            tft.setTextSize(2);
            tft.setTextColor(RED, BLACK);
            // Flash display
            if (timeToGo % 2 == 0 && MCP7940.isAlarm(0))
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
          tft.setCursor(43, 95);
          tft.setTextSize(1);
          tft.setTextColor(BLUE, BLACK);
          sprintf(timeBuff,"%02d:%02d:%02d", hr, min, sec);
          tft.print(timeBuff);
      } // void drawTime


    void drawInfo(Adafruit_SSD1351 tft, max1720x gauge)
      {
        tft.setTextColor(BLUE, BLACK);
        tft.setTextSize(1);
        tft.setCursor(5, 110);
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
        tft.setCursor(5, 118);
        //tft.print("TTE ");
        //float tte = gauge.getTTE();
        //tte = tte/3600;
        //tft.print(tte);
        //tft.print(" TTF ");
        //float ttf = gauge.getTTF();
        //ttf = ttf/3600;
        //tft.print(ttf);
        float cap = gauge.getCapacity();
        cap = cap*2;
        int r_cap = round(cap);
        tft.print("Cap ");
        tft.print(r_cap);
        float mA = gauge.getCurrent();
        tft.print(" mA ");
        tft.print(abs(mA));
        //tft.setCursor(50, 118);
        //tft.print("HL ");
        //tft.print(highlighted);
        //tft.print(" TTE ");
        //float tte = gauge.getTTE();
        //tte = tte/3600;
        //int r_tte = round(tte);
        //tft.print(r_tte);
        //tft.print(" RAM ");
        //tft.print(freeRam());
        //tft.print(" Mode ");
        //tft.print(displayMode);
        tft.setTextSize(2);
      } // void drawInfo()

    int freeRam ()
      {
        extern int __heap_start, *__brkval;
        int v;
        return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
      } // freeRam
  };
