/**
 * @file main.cpp
 * @author Martin Ming MMAGMMA
 * @brief
 * @version 0.1
 * @date 10.04.2025
 * 
 * @copyright Copyright (c) maxon motor ag 2025
 */

#include <Arduino.h>
#include "definitions.h"
//#include <Adafruit_GFX.h>
//#include <Adafruit_ST7789.h>
//#include <SPI.h>
#include <ESP32Time.h>
#include <PCF8574.h>
#include <DHT20.h>
#include <RTCC_MCP7940N.h>
#include <Adafruit_VEML7700.h>
//#include <Fonts/FreeSansBold24pt7b.h>
//#include <Fonts/FreeSansBold18pt7b.h>
//#include <Fonts/FreeSansBold12pt7b.h>
//#include <Fonts/FreeSansBold9pt7b.h>
#include "rgbLightRing.h"
#include "tftDisplay.h"

/*Pin definitions*/

/*LED's*/
#define PIN_DO_LED_Data 4
#define PIN_DO_LED_Clk 2
#define PIN_DO_LED_Clear 3
#define PIN_PWM_LED_Pwm 1

/*Button V2*/
/*#define BUTTON_MENU 4
#define BUTTON_BELL 5
#define BUTTON_UP 6
#define BUTTON_DOWN 7
#define PRESSED false
#define RELEASED true*/

/*Button V3*/
#define BUTTON_MENU 5
#define BUTTON_BELL 4
#define BUTTON_UP 7
#define BUTTON_DOWN 6
#define PRESSED false
#define RELEASED true

/*Button LED's V2*/
/*#define BUTTON_LED_MENU 0
#define BUTTON_LED_BELL 1
#define BUTTON_LED_PLUS 2
#define BUTTON_LED_MINUS 3*/

/*Button LED's V3*/
#define BUTTON_LED_MENU 1
#define BUTTON_LED_BELL 0
#define BUTTON_LED_PLUS 3
#define BUTTON_LED_MINUS 2

/*I2C*/
#define PIN_I2C_SDA 5
#define PIN_I2C_SCL 6
/*Port Expander*/
PCF8574 pcf8574(0x39);
/*Sensor Feuchtigkeit und Temperatur*/
DHT20 DHT; 
/*Sensor Licht*/
Adafruit_VEML7700 veml = Adafruit_VEML7700();
/*Echtzeituhr*/
RTCC_MCP7940N rtc_ext;

/*Time definition*/
ESP32Time rtc(0);   // ESP Zeit
rtcc_time timeIntern; // Echtseituhr Zeit
rtcc_time timeInternFailDown; // Echtseituhr Zeit
rtcc_time timeInternFailUp; // Echtseituhr Zeit
rtcc_time timeInternAlarm1; // Echtseituhr Zeit
bool pwrOutage = false;
bool setTimeOK = false;
uint alarmSetMin = 0;
uint alarmSetHr = 7;
bool alarmIsActive = false;
bool alarmOn = false;

/*Display*/
#define PIN_DO_TFT_Reset -1 //Reset wird nicht umbedingt benötigt deshalb wird dieser Pin für TFT_DC verwendet
#define PIN_SPI_TFT_DC 43
#define PIN_SPI_TFT_MOSI 9
#define PIN_SPI_TFT_CS 8
#define PIN_SPI_TFT_CLK 7
#define PIN_PWM_TFT_Blight 44
TftDisplay display(PIN_SPI_TFT_CS, PIN_SPI_TFT_DC, PIN_SPI_TFT_MOSI, PIN_SPI_TFT_CLK, PIN_DO_TFT_Reset);
//Adafruit_ST7789 tft = Adafruit_ST7789(PIN_SPI_TFT_CS, PIN_SPI_TFT_DC, PIN_SPI_TFT_MOSI, PIN_SPI_TFT_CLK, PIN_DO_TFT_Reset);

/*PWM Config*/
const int PWM_CHANNEL_LED = 0;
const int PWM_CHANNEL_BACKLIGHT = 1;
const int PWM_FREQ = 5000;
const int PWM_RESOLUTION = 8;
int brightnessTFT = 20;
int brightnessLED = 20;

/*States*/
/*#define STATE_NOTIMESET 0
#define STATE_TIMESET 1
#define STATE_SETSETTINGMODE 50
#define STATE_TIMESETMIN 51
#define STATE_TIMESETHR 52
#define STATE_TIMESETDAY 53
#define STATE_TIMESETMONTH 54
#define STATE_TIMESETYEAR 55
#define STATE_ALARMSETMIN 60
#define STATE_ALARMSETHR 61
#define STATE_SETBRIGHTNESS 80

#define SETTING_CHAPTER_TIME 0
#define SETTING_CHAPTER_ALARMTIME 1
#define SETTING_CHAPTER_DISPLAYBRIGHTNESS 3
#define SETTING_CHAPTER_REFRESHRATE 4
#define SETTING_CHAPTER_EXIT 2*/

int state = STATE_NOTIMESET;
uint settingChapter = SETTING_CHAPTER_TIME;
uint oldSettingChapter = -1;
bool secondChanged = true;
bool minuteChanged = true;
bool hourChanged = true;
bool dayChanged = true;

RgbLedRing ledRing(PIN_DO_LED_Data, PIN_DO_LED_Clk, PIN_DO_LED_Clear );  // LED an Pin 8

void exttimeToEsptime();
void exttimePrintToSerial();
//void showActualTime();
void printTime();
void printDate();
void printAlarmTime();
void printSetting();
void printTimeOutage();
//void printTempHum(float temperatutre, float humidity, uint16_t bgcolor);
//void calculateBrightness(uint8_t *calculatetDimmingValue);
void setButtonLeds();
void setButtonLeds(bool lMenu, bool lBell, bool lPlus, bool lMinus);
void setBrightness(float dimmingValue);
void showBrightness(int brightness);

void setup() {
  /*Init shift register für die ansteuerung der LED's im Ring*/
  ledRing.begin();
  
  /*Init der PWM Signale für Helligkeit LED's und Display */
  ledcSetup(PWM_CHANNEL_BACKLIGHT, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(PIN_PWM_TFT_Blight, PWM_CHANNEL_BACKLIGHT);
  ledcWrite(PWM_CHANNEL_BACKLIGHT, 255);
  ledcSetup(PWM_CHANNEL_LED, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(PIN_PWM_LED_Pwm, PWM_CHANNEL_LED);
  ledcWrite(PWM_CHANNEL_LED, 255);

  delay(500);// warten das Upload ohne Tasterdruck funktioniert.
  /*Serial(USB) for debug*/
  Serial.begin(115200);

  /*I2C*/
  Wire.begin();
  Wire.setClock(100000);
  if (pcf8574.begin()){
		//Serial.println("OK");
	}else{
		//Serial.println("failed");
	}
  /*Init Portexpander*/
  pcf8574.write8(0xFF); //Alle Pins auf High setzen.
  pcf8574.setButtonMask(0b11110000);
  setButtonLeds(1,1,1,1);

  /*Init Lichtsensor*/
  veml.begin();

  /*Init Echtzeituhr*/
  pwrOutage = rtc_ext.HasPowerFailed(&timeInternFailDown, &timeInternFailUp);

  rtc_ext.SetOscillatorEnabled(true);
  rtc_ext.SetExternalOscillatorEnabled(false);
  rtc_ext.SetSquareWaveOutputState(false);
  rtc_ext.SetGPOState(false);
  rtc_ext.SetBatteryEnabled(true);
  rtc_ext.Set24HourTimeEnabled(true);
  rtc_ext.SetAlarm1Enabled(false);
  rtc_ext.ClearAlarm1Flag();
  rtc_ext.SetAlarm2Enabled(false);
  rtc_ext.ClearAlarm2Flag();

  
  setButtonLeds(1,1,1,0);

  /*Init Display*/
  display.begin();
  setButtonLeds(1,1,0,0);

  delay(500);
  rtc_ext.SetGPOState(true);
  setButtonLeds(1,0,0,0);

  delay(1500);

  Serial.println("Start");

  rtc_ext.ReadTime(&timeIntern);
  exttimeToEsptime();
  exttimePrintToSerial();
  //if(pwrOutage){
    //rtc_ext.ClearPowerFailedFlag();
    printTimeOutage();
  //}
  
  /*Lightring Show*/
  ledRing.showTurningPoint(10);
  ledRing.showTurningPointWhite(10);
  ledRing.showTurningWorm(10);
  display.clearScreen(ST77XX_BLACK);
  printTime();
  printDate();
  display.printTempHum(0.0, 0.0, ST77XX_BLACK);
  setButtonLeds(0,0,0,0);

}

void loop() {
  static int oldSecond = -1;
  static int oldMinute = -1;
  static int oldDay = -1;
  static int settingCounter = 0;
  static bool knobUpDownPressed = 0;
  static bool knobMenuPressed = false;
  static bool knobBellPressed = false;
  static float sensorValueTemperature = 0.0;
  static float sensorValueHuminity = 0.0;
  static float sensorValueBrightness= 0.0;
  static uint8_t brightnessDimmingValue;
  int actSecond = rtc.getSecond();
  int actMinute = rtc.getMinute();
  int actDay = rtc.getDay();
  int SettingWaitCounter = 0;
  long newTime = 0 ;
  uint8_t inputValues = 0;
  if(oldSecond != actSecond){
    secondChanged = true;
    oldSecond = actSecond;
  }else{
    secondChanged = false;
  }
  if(oldMinute != actMinute){
    minuteChanged = true;
    oldMinute = actMinute;
  }else{
    minuteChanged = false;
  }
  if(oldDay != actDay){
    dayChanged = true;
    oldDay = actDay;
  }else{
    dayChanged = false;
  }

  /*Aktualisierungen Zeitgesteuert*/
  if(secondChanged & (actSecond % 1 == 0)){
        sensorValueBrightness = veml.readLux();
  }
  if(secondChanged & (actSecond % 2 == 0)){
        DHT.read();
        sensorValueTemperature = DHT.getTemperature();
        sensorValueHuminity = DHT.getHumidity();
        /*Serial.print(sensorValueTemperature);
        Serial.print(" ; ");
        Serial.print(sensorValueHuminity);
        Serial.print(" ; ");
        Serial.print(sensorValueBrightness);
        Serial.println(" ; ");
        exttimePrintToSerial();*/
  }

    /*Verarbeitung +/- Tastenzustand*/
  if(pcf8574.readButton(BUTTON_UP) == PRESSED && knobUpDownPressed == false){
    knobUpDownPressed = true;
    settingCounter++;
    Serial.print("UP_pressed Value: ");
    Serial.println(settingCounter);
  }
  if(pcf8574.readButton(BUTTON_DOWN) == PRESSED && knobUpDownPressed == false){
    knobUpDownPressed = true;
    settingCounter--;
    Serial.print("UP_pressed Value: ");
    Serial.println(settingCounter);
  }

    /*Verarbeitung Menü Tastenzustand*/
  if(pcf8574.readButton(BUTTON_MENU) == PRESSED && knobMenuPressed == false){
    while (pcf8574.readButton(BUTTON_MENU) == PRESSED && state == STATE_TIMESET) { //wartezeit langes drücken Menu für Settingstart
      SettingWaitCounter++;
      if(SettingWaitCounter >= 100){
        knobMenuPressed = true;
        setButtonLeds();
        display.showSettingBar(SettingWaitCounter, ST77XX_BLACK);
        printSetting();
        break;
      }
      display.showSettingBar(SettingWaitCounter, ST77XX_RED);
      Serial.println(SettingWaitCounter);
      delay(20);
    }
    if(SettingWaitCounter > 0 && state == STATE_TIMESET){  //Zurücksetzen des roten Wartebalkens
      display.showSettingBar(SettingWaitCounter, ST77XX_BLACK);
    }
    if(pcf8574.readButton(BUTTON_MENU) == PRESSED && state != STATE_TIMESET){
      knobMenuPressed = true;
        setButtonLeds();
      delay(10);
    }
  }

    /*Verarbeitung Glocke (Alarm) Tastenzustand*/
  if(pcf8574.readButton(BUTTON_BELL) == PRESSED && knobBellPressed == false){
    while (pcf8574.readButton(BUTTON_BELL) == PRESSED && knobBellPressed == false) { //state == STATE_TIMESET
      SettingWaitCounter++;
      if(SettingWaitCounter >= 50){
        knobBellPressed = true;
        alarmIsActive = !alarmIsActive;
        setButtonLeds();
        display.showSettingBar(SettingWaitCounter*2, ST77XX_BLACK);
        printAlarmTime();
        break;
      }
      display.showSettingBar(SettingWaitCounter*2, ST77XX_RED);
      delay(20);
    }
  }  

  switch (state) {
    case STATE_NOTIMESET:
      if(secondChanged){
        ledRing.showActualTime(rtc.getHour(false), rtc.getMinute(), rtc.getSecond()); 

        if((actSecond % 2) == 1){
          setBrightness(20.0);
        }else{
          setBrightness(sensorValueBrightness);
          showBrightness(sensorValueBrightness / 10);
        }
        if((actSecond % 10) == 0){
          display.printTempHum(sensorValueTemperature, sensorValueHuminity, ST77XX_BLACK);
        }
      }
      if(minuteChanged){printTime(); display.printTimeWarning();}
      if(dayChanged){printDate();}
      break;

    case STATE_TIMESET:
      if(secondChanged){
        ledRing.showActualTime(rtc.getHour(false), rtc.getMinute(), rtc.getSecond());
        if(!alarmOn){
          setBrightness(sensorValueBrightness);
        }else{
          if((actSecond % 2) == 1){
            setBrightness(20.0);
          }else{
            setBrightness(100.0);
          }
        }
        
        if(actSecond % 2 == 0){
          display.printTempHum(sensorValueTemperature, sensorValueHuminity, ST77XX_BLACK);
        }
        showBrightness(sensorValueBrightness / 10);
      }
      if(minuteChanged){printTime();}
      if(dayChanged){printDate();}
      break;

    case STATE_SETSETTINGMODE:
      if(setTimeOK){
        settingChapter = settingCounter % 3;
      }else{
        settingChapter = SETTING_CHAPTER_TIME;
      }
      printSetting();
      break;

    case STATE_TIMESETMIN:
      newTime = rtc.getLocalEpoch() + (settingCounter * 60);
      rtc.setTime(newTime);
      settingCounter = 0;
      printTime();
      break;
    case STATE_TIMESETHR:
      newTime = rtc.getLocalEpoch() + (settingCounter * 3600);
      rtc.setTime(newTime);
      settingCounter = 0;
      printTime();
      break;
    case STATE_TIMESETDAY:
      newTime = rtc.getLocalEpoch() + (settingCounter * 86400);
      rtc.setTime(newTime);
      settingCounter = 0;
      printDate();
      break;
    case STATE_TIMESETMONTH:
      newTime = rtc.getMonth() + settingCounter + 1;
      rtc.setTime(rtc.getSecond(), rtc.getMinute(), rtc.getHour(true), rtc.getDay(), newTime, rtc.getYear());
      settingCounter = 0;
      printDate();
      break;
    case STATE_TIMESETYEAR:
      newTime = rtc.getYear() + settingCounter;
      rtc.setTime(rtc.getSecond(), rtc.getMinute(), rtc.getHour(true), rtc.getDay(), rtc.getMonth()+1, newTime);
      settingCounter = 0;
      printDate();
      break;

    case STATE_ALARMSETMIN:
      if(settingCounter < 0 && alarmSetMin == 0){
        alarmSetMin = 59;
      }else{
        alarmSetMin = alarmSetMin + settingCounter;
        if(alarmSetMin >=60){
          alarmSetMin = 0;
        }
      }
      settingCounter = 0;
      printTime();
      break;
    case STATE_ALARMSETHR:
    if(settingCounter < 0 && alarmSetHr == 0){
        alarmSetHr = 23;
      }else{
        alarmSetHr = alarmSetHr + settingCounter;
        if(alarmSetHr >=24){
          alarmSetHr = 0;
        }
      }
      settingCounter = 0;
      printTime();
      break;
    /*case STATE_SETBRIGHTNESS:
      //displayBrightness = (newPosition % 9)*31+7;
      //setBackLight(displayBrightness, 1);
      //printBrightness();
      Serial.println("Brightnesssetting");
      break;*/
    default:
      // Statement(s)
      break; // Wird nicht benötigt, wenn Statement(s) vorhanden sind
  }

  //cange state for settings
  if(knobMenuPressed == true){
    //settingCounter = 0;
    switch (state) {
      case STATE_NOTIMESET:
        state = STATE_SETSETTINGMODE;
        break;
      case STATE_TIMESET:
        state = STATE_SETSETTINGMODE;
        setButtonLeds();
        break;
      case STATE_SETSETTINGMODE:
        display.clearScreen(ST77XX_YELLOW);
        Serial.print("setting Chapter: ");
        Serial.println(settingChapter);
        switch (settingChapter) {
          case SETTING_CHAPTER_TIME:
            state = STATE_TIMESETMIN;        
            printTime();
            printDate();
            break;
          case SETTING_CHAPTER_ALARMTIME:
            state = STATE_ALARMSETMIN;
            break;
          /*case SETTING_CHAPTER_DISPLAYBRIGHTNESS:
            state = STATE_SETBRIGHTNESS;
            break;*/
          case SETTING_CHAPTER_EXIT:
            if(setTimeOK == 0){
              state = STATE_NOTIMESET;
            }else{
              state = STATE_TIMESET;
            }
            display.clearScreen(ST77XX_BLACK);
            printTime();
            printDate();
            printAlarmTime();
            break;
          default:
            // Statement(s)
            break;
        }
        oldSettingChapter = -1;
        break;

      case STATE_TIMESETMIN:
        state = STATE_TIMESETHR;
        break;
      case STATE_TIMESETHR:
        state = STATE_TIMESETDAY;
        printTime();
        break;
      case STATE_TIMESETDAY:
        state = STATE_TIMESETMONTH;
        break;
      case STATE_TIMESETMONTH:
        state = STATE_TIMESETYEAR;
        break;
      case STATE_TIMESETYEAR:      
        timeIntern.secone = rtc.getSecond() % 10;
        timeIntern.secten = rtc.getSecond() / 10;
        timeIntern.minone = rtc.getMinute() % 10;
        timeIntern.minten = rtc.getMinute() / 10;
        timeIntern.hrone = rtc.getHour(true) % 10;
        timeIntern.hrten = rtc.getHour(true) / 10;
        timeIntern.dateone = rtc.getDay() % 10;
        timeIntern.dateten = rtc.getDay() / 10;
        timeIntern.mthone = (rtc.getMonth() + 1) % 10;
        timeIntern.mthten = (rtc.getMonth() + 1) / 10;
        timeIntern.yrone = rtc.getYear()  % 10;
        timeIntern.yrten = (rtc.getYear() - 2000) / 10;
        rtc_ext.SetTime(&timeIntern);

        state = STATE_TIMESET;
        setButtonLeds();
        setTimeOK = true;
        display.clearScreen(ST77XX_BLACK);
        printTime();
        printDate();
        printAlarmTime();
        break;
      case STATE_ALARMSETMIN:
        state = STATE_ALARMSETHR;
        break;
      case STATE_ALARMSETHR:
        alarmIsActive = 1;
        state = STATE_TIMESET;
        setButtonLeds();
        display.clearScreen(ST77XX_BLACK);
        printTime();
        printDate();
        printAlarmTime();
        break;
      /*case STATE_SETBRIGHTNESS:
        //newPosition = 0;
        state = STATE_TIMESET;
        tft.fillScreen(ST77XX_WHITE);
        printTime();
        printDate();
        printAlarmTime();
        break;*/
      default:
        state = STATE_NOTIMESET;
        break; // Wird nicht benötigt, wenn Statement(s) vorhanden sind
        
    }
    delay(10);
  }
  delay(10);
  
  if(pcf8574.readButton(BUTTON_MENU) == RELEASED && knobMenuPressed == true){
    display.showSettingBar(100, ST77XX_BLACK); //löscht wartebalken
    knobMenuPressed = false;
    Serial.println("Menu released");
  }
  if(pcf8574.readButton(BUTTON_BELL) == RELEASED && knobBellPressed == true){
    display.showSettingBar(100, ST77XX_BLACK); //löscht wartebalken
    knobBellPressed = false;
    Serial.println("Bell released");
  }

  if(pcf8574.readButton(BUTTON_UP) == RELEASED && pcf8574.readButton(BUTTON_DOWN) == RELEASED && knobUpDownPressed == true){
    knobUpDownPressed = false;
    Serial.println("Up/Down released");
  }

  //Check is time for alarm and alarm activated
  if(alarmSetMin == rtc.getMinute() && alarmSetHr == rtc.getHour(true) && alarmIsActive && state == STATE_TIMESET && alarmOn == false){
    alarmOn = true;
    rtc_ext.SetGPOState(false);
    Serial.println("Alarm Start");
  }

  //Check is alarmbutton presst for stop alarm
  if(pcf8574.readButton(BUTTON_BELL) == PRESSED && alarmOn == true){
    alarmOn = 0;
    rtc_ext.SetGPOState(true);
    Serial.println("Alarm Stop");
  }
  
  //secondChanged = false;
  //minuteChanged = false;
  delay(10);
}

/*
*Kopiert die Externe Uhr in die ESP Uhr
*/
void exttimeToEsptime(){
  int tempSecond = (timeIntern.secten * 10) + timeIntern.secone;
  int tempMinute = (timeIntern.minten * 10) + timeIntern.minone;
  int tempHour = (timeIntern.hrten * 10) + timeIntern.hrone;
  int tempDay = (timeIntern.dateten * 10) + timeIntern.dateone; 
  int tempMonth = (timeIntern.mthten * 10) + timeIntern.mthone; 
  int tempYear = 2000 + (timeIntern.yrten * 10) + timeIntern.yrone;
  rtc.setTime(tempSecond, tempMinute, tempHour, tempDay, tempMonth, tempYear);
}


void exttimePrintToSerial(){
  char buff[20];
  rtc_ext.ReadTime(&timeIntern);

  // Print time every second
  sprintf(buff, "%u%u-%u%u-%u%u %u%u:%u%u:%u%u", timeIntern.dateten, timeIntern.dateone, timeIntern.mthten, timeIntern.mthone, timeIntern.yrten,
      timeIntern.yrone, timeIntern.hrten, timeIntern.hrone, timeIntern.minten, timeIntern.minone, timeIntern.secten, timeIntern.secone);
  Serial.println(buff);
  Serial.print("ESPTime: ");
  Serial.println(rtc.getDateTime());
}

/**
 * @brief Schreibt die aktuelle Zeit und das Datum auf das Display
 */
void printTime(){
  char timeString[6] = " ";
  switch (state) {
  case STATE_NOTIMESET:
      rtc.getTime("%H:%M").toCharArray(timeString, 7);
      display.printTime(timeString, ST77XX_RED, ST77XX_YELLOW);  
    break;
  case STATE_TIMESET:
      rtc.getTime("%H:%M").toCharArray(timeString, 7);
      display.printTime(timeString, ST77XX_RED, ST77XX_BLACK);  
    break;
  case STATE_TIMESETMIN:
      //setBackLight(255, 1);
      rtc.getTime("%H:%M").toCharArray(timeString, 7);
      display.printTime(timeString, state);
      ledRing.showActualTime(rtc.getHour(false), rtc.getMinute(), rtc.getSecond());
    break;
  case STATE_TIMESETHR:
      rtc.getTime("%H:%M").toCharArray(timeString, 7);
      display.printTime(timeString, state);
      ledRing.showActualTime(rtc.getHour(false), rtc.getMinute(), rtc.getSecond());
    break;
  case STATE_ALARMSETMIN:
      sprintf(timeString, "%02d:%02d", alarmSetHr, alarmSetMin);
      display.printTime(timeString, state);
    break;
  case STATE_ALARMSETHR:
      sprintf(timeString, "%02d:%02d", alarmSetHr, alarmSetMin);
      display.printTime(timeString, state);
    break;
  default:
    // Statement(s)
    break; // Wird nicht benötigt, wenn Statement(s) vorhanden sind
  }
}

void printDate(){
  char dateString[11] = " ";
  rtc.getTime("%d.%m.%Y").toCharArray(dateString, 11);
  switch (state) {
  case STATE_NOTIMESET:
    display.printDate(dateString, ST77XX_RED, ST77XX_YELLOW);
    break;
  case STATE_TIMESET:
    display.printDate(dateString, ST77XX_RED, ST77XX_BLACK);
    break;
  case STATE_TIMESETDAY:
    display.printDate(dateString, state);
    break;
  case STATE_TIMESETMONTH:
    display.printDate(dateString, state);
    break;
  case STATE_TIMESETYEAR:
    display.printDate(dateString, state);
    break;
  default:
    // Statement(s)
    break; // Wird nicht benötigt, wenn Statement(s) vorhanden sind
  }
}

void printAlarmTime(){
  char alarmString[16] = " ";
  sprintf(alarmString, "Weckzeit: %02d:%02d", alarmSetHr, alarmSetMin);
  display.printAlarmTime(alarmString, alarmIsActive);
}

void printSetting(){ //TODO: Anpassen mit Texten zum ausgeben
  if(oldSettingChapter != settingChapter){
    oldSettingChapter = settingChapter;
    display.showSettingMenu(settingChapter);
  }
}

void printTimeOutage(){
  char buff[20];
  Serial.print("    From: ");
	sprintf(buff, "%u%u-%u%u-%u%u %u%u:%u%u:%u%u", timeInternFailDown.dateten, timeInternFailDown.dateone, timeInternFailDown.mthten, timeInternFailDown.mthone, timeInternFailDown.yrten,
			timeInternFailDown.yrone, timeInternFailDown.hrten, timeInternFailDown.hrone, timeInternFailDown.minten, timeInternFailDown.minone, timeInternFailDown.secten, timeInternFailDown.secone);
	Serial.println(buff);
	Serial.print("    To: ");
	sprintf(buff, "%u%u-%u%u-%u%u %u%u:%u%u:%u%u", timeInternFailUp.dateten, timeInternFailUp.dateone, timeInternFailUp.mthten, timeInternFailUp.mthone, timeInternFailUp.yrten,
			timeInternFailUp.yrone, timeInternFailUp.hrten, timeInternFailUp.hrone, timeInternFailUp.minten, timeInternFailUp.minone, timeInternFailUp.secten, timeInternFailUp.secone);
	Serial.println(buff);
}

void setButtonLeds(){
  if(state != STATE_TIMESET ){
    pcf8574.write(BUTTON_LED_MENU, false);
    pcf8574.write(BUTTON_LED_PLUS, false);
    pcf8574.write(BUTTON_LED_MINUS, false);
  }else{
    pcf8574.write(BUTTON_LED_MENU, true);
    pcf8574.write(BUTTON_LED_PLUS, true);
    pcf8574.write(BUTTON_LED_MINUS, true);
  }
  pcf8574.write(BUTTON_LED_BELL, !alarmIsActive);
}

void setButtonLeds(bool lMenu, bool lBell, bool lPlus, bool lMinus){
  pcf8574.write(BUTTON_LED_MENU, !lMenu);
  pcf8574.write(BUTTON_LED_BELL, !lBell);
  pcf8574.write(BUTTON_LED_PLUS, !lPlus);
  pcf8574.write(BUTTON_LED_MINUS, !lMinus);
}

void setBrightness(float dimmingValue){
  uint8_t dimmingValueTFT = 0;
  uint8_t dimmingValueLED = 0;

  if(dimmingValue > 500){
          dimmingValueTFT = 100;
          dimmingValueLED = 100;
        }else if(dimmingValue < 20){
          dimmingValueTFT = 2;
          dimmingValueLED = 2;
        }else{
          dimmingValueTFT = dimmingValue / 5;
          dimmingValueLED = dimmingValue / 5;
        } 
  ledcWrite(PWM_CHANNEL_BACKLIGHT, dimmingValueTFT);
  ledcWrite(PWM_CHANNEL_LED, dimmingValueLED);
}

void showBrightness(int brightness){
  if(brightness > 100){brightness = 100;}
  //tft.fillRect(235,30,5, 100-brightness,ST77XX_WHITE);
  //tft.fillRect(235,130,5, -brightness,ST77XX_ORANGE);
}
