#ifndef TFT_DISPLAY_H
#define TFT_DISPLAY_H

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>


class TftDisplay {
  public:
    /**
     * Konstruktor
     * @param pinTftCS Anschluss Schieberegister Data Pin
     * @param pinTftDC Anschluss Schieberegister Clock Pin
     * @param pinTftMOSI Anschluss Schieberegister Clear Pin
     * pinTftCLK
     * pinTftRESET
     */
    TftDisplay(uint8_t pinTftCS, uint8_t pinTftDC, uint8_t pinTftMOSI, uint8_t pinTftCLK, uint8_t pinTftRESET);  // Pin wird beim Erstellen übergeben
        //Adafruit_ST7789 tft = Adafruit_ST7789(_pinTftCS, _pinTftDC, _pinTftMOSI, _pinTftCLK, _pinTftRESET);
    void begin();
    void clearScreen(uint16_t bgcolor);
    void printTime(const char* timeString, uint16_t txtColor, uint16_t bgColor);
    void printTime(const char* timeString, int state);
    void printDate(const char* dateString, uint16_t txtColor, uint16_t bgColor);
    void printDate(const char* dateString, int state);
    void printAlarmTime(const char* alarmString, bool alarmActive);
    void printTempHum(float temperature, float humidity, uint16_t bgColor);
    void showSettingBar(int waitValue, uint16_t barColor);
    void printTimeWarning();


  private:
    uint8_t _pinTftCS;  // Pin Data Anschluss
    uint8_t _pinTftDC;  // Pin Clock Anschluss
    uint8_t _pinTftMOSI;  // Pin Clear Anschluss
    uint8_t _pinTftCLK;  // Pin Clock Anschluss
    uint8_t _pinTftRESET;  // Pin Clear Anschluss
    //Adafruit_ST7789 tft = Adafruit_ST7789(_pinTftCS, _pinTftDC, _pinTftMOSI, _pinTftCLK, _pinTftRESET);
    Adafruit_ST7789 tft;

};

#endif