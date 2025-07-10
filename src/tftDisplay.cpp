#include "tftDisplay.h"

#include "definitions.h"


/*
*initialisierung der Pins
*/
TftDisplay::TftDisplay(uint8_t cs, uint8_t dc, uint8_t mosi, uint8_t clk, uint8_t rst)
  : _pinTftCS(cs), _pinTftDC(dc), _pinTftMOSI(mosi), _pinTftCLK(clk), _pinTftRESET(rst),
    tft(cs, dc, mosi, clk, rst)  // Direktes Objekt in Initialisierungsliste
{}

void TftDisplay::begin() {
  tft.init(135, 240);   //Init ST7789 240x135px
  tft.setSPISpeed(40000000);
  tft.setRotation(3);
  tft.fillScreen(ST77XX_BLACK);
  tft.setFont(&FreeSansBold24pt7b);
  tft.setTextColor(ST77XX_RED, ST77XX_BLACK);
  tft.setCursor(50,40);
  tft.print("maxon");
  tft.setFont(&FreeSansBold9pt7b);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setCursor(50,70);
  tft.print("Berufsbildung");
}

void TftDisplay::clearScreen(uint16_t bgcolor){
  tft.fillScreen(bgcolor);
}

void TftDisplay::printTime(const char* timeString, uint16_t txtColor, uint16_t bgColor){
    tft.setFont(&FreeSansBold24pt7b);
    tft.setTextColor(txtColor, bgColor);
    tft.fillRect(60,5,120,40,bgColor);
    tft.setCursor(60,40);
    tft.print(timeString);
}

void TftDisplay::printTime(const char* timeString, int state){
    tft.setFont(&FreeSansBold24pt7b);
    switch (state) {
    case STATE_NOTIMESET:
        break;
    case STATE_TIMESET:
        break;
    case STATE_TIMESETMIN:
        tft.fillRect(125,5,50,40,ST77XX_CYAN);
        break;
    case STATE_TIMESETHR:
        tft.fillRect(60,5,50,40,ST77XX_CYAN);
        break;
    case STATE_ALARMSETMIN:
        tft.fillRect(125,5,50,40,ST77XX_CYAN);
        break;
    case STATE_ALARMSETHR:
        tft.fillRect(60,5,50,40,ST77XX_CYAN);
        break;
    default:
        // Statement(s)
        break; // Wird nicht benötigt, wenn Statement(s) vorhanden sind
    }
    tft.setTextColor(ST77XX_RED, ST77XX_BLACK);
    tft.setCursor(60,40);
    tft.print(timeString);
}

void TftDisplay::printDate(const char* dateString, uint16_t txtColor, uint16_t bgColor){
    tft.setFont(&FreeSansBold18pt7b);
    tft.setTextColor(txtColor, bgColor);
    tft.fillRect(40,81,170,-27,bgColor);
    tft.setCursor(40, 80);
    tft.print(dateString);
}

void TftDisplay::printDate(const char* dateString, int state){
    tft.setFont(&FreeSansBold18pt7b);
    switch (state) {
        case STATE_NOTIMESET:
            tft.fillRect(40,81,170,-27,ST77XX_YELLOW);
            break;
        case STATE_TIMESET:
            tft.fillRect(40,81,170,-27,ST77XX_WHITE);
            break;
        case STATE_TIMESETDAY:
            tft.fillRect(40,81,40,-27,ST77XX_CYAN);
            break;
        case STATE_TIMESETMONTH:
            tft.fillRect(85,81,40,-27,ST77XX_CYAN);
            break;
        case STATE_TIMESETYEAR:
            tft.fillRect(130,81,80,-27,ST77XX_CYAN);
            break;
        default:
            // Statement(s)
            break; // Wird nicht benötigt, wenn Statement(s) vorhanden sind
        }
    tft.setTextColor(ST77XX_RED, ST77XX_BLACK);
    tft.setCursor(40, 80);
    tft.print(dateString);
}

void TftDisplay::printAlarmTime(const char* alarmString, bool alarmActive){  
    tft.setFont(&FreeSansBold9pt7b);
    tft.setTextColor(ST77XX_RED, ST77XX_BLACK);
    tft.fillRect(20,119,186,22,ST77XX_BLACK);
    tft.setCursor(20, 131);
  if(alarmActive){
    tft.fillRect(20,119,186,22,ST77XX_YELLOW);
    tft.print(alarmString);
  }else{
    tft.fillRect(20,119,186,22,ST77XX_BLACK);
    tft.print("Wecker aus");
  }
}

void TftDisplay::printTempHum(float temperature, float humidity, uint16_t bgColor){ //TODO: Anpassen mit Texten zum ausgeben
  tft.setFont(&FreeSansBold18pt7b);
  tft.setTextColor(ST77XX_RED, ST77XX_BLACK);
  tft.fillRect(0,115,110,-25, bgColor);
  tft.setCursor(5, 115);
  tft.print(temperature, 1);
  tft.print("*C");
  
  tft.fillRect(120,115,115,-25, bgColor);
  tft.setCursor(120, 115);
  tft.print(humidity, 1);
  tft.print(" %");
}

void TftDisplay::showSettingBar(int waitValue, uint16_t barColor){
    tft.fillRect(230,2,5,waitValue,barColor);
}

void TftDisplay::printTimeWarning(){ //TODO: Anpassen mit Texten zum ausgeben
  tft.setFont(&FreeSansBold18pt7b);
  tft.fillRect(0,30,30,-30,ST77XX_YELLOW);
  tft.setTextColor(ST77XX_RED, ST77XX_YELLOW);
  tft.setCursor(9, 27);
  tft.print("!");
  tft.setFont(&FreeSansBold9pt7b);
  tft.setCursor(20, 131);
  tft.print("Zeit nicht eingestellt!!");
}