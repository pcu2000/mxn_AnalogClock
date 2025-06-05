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
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <ESP32Time.h>
#include <PCF8574.h>
#include <DHT20.h>
#include <RTCC_MCP7940N.h>
#include <Adafruit_VEML7700.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>

/*Pin definitions*/

/*LED's*/
#define PIN_DO_LED_Data 4
#define PIN_DO_LED_Clk 2
#define PIN_DO_LED_Clear 3
#define PIN_PWM_LED_Pwm 1
const int redArray = 2;
const int blueArray = 1;
const int greenArray = 0;

/*Button*/
#define BUTTON_MENU 4
#define BUTTON_BELL 5
#define BUTTON_UP 6
#define BUTTON_DOWN 7
#define PRESSED false
#define RELEASED true

/*Button LED's*/
#define BUTTON_LED_MENU 0
#define BUTTON_LED_BELL 1
#define BUTTON_LED_PLUS 2
#define BUTTON_LED_MINUS 3

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
Adafruit_ST7789 tft = Adafruit_ST7789(PIN_SPI_TFT_CS, PIN_SPI_TFT_DC, PIN_SPI_TFT_MOSI, PIN_SPI_TFT_CLK, PIN_DO_TFT_Reset);

/*PWM Config*/
const int PWM_CHANNEL_LED = 0;
const int PWM_CHANNEL_BACKLIGHT = 1;
const int PWM_FREQ = 5000;
const int PWM_RESOLUTION = 8;
int brightnessTFT = 20;
int brightnessLED = 20;

/*States*/
#define STATE_NOTIMESET 0
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
#define SETTING_CHAPTER_DISPLAYBRIGHTNESS 2
#define SETTING_CHAPTER_REFRESHRATE 3
#define SETTING_CHAPTER_EXIT 4

int state = STATE_NOTIMESET;
int settingChapter = SETTING_CHAPTER_TIME;
int oldSettingChapter = -1;
bool secondChanged = true;
bool minuteChanged = true;
bool hourChanged = true;
bool dayChanged = true;

void exttimeToEsptime();
void exttimePrintToSerial();
void showTurningPoint(int speed);
void showActualTime();
void printTime();
void printDate();
void printAlarmTime();
void printSetting(int pointPlace);
void printTimeWarning();
void printTimeOutage();
void printTempHum(float temperatutre, float humidity, uint16_t bgcolor);
//void calculateBrightness(uint8_t *calculatetDimmingValue);
void setButtonLeds();
void setButtonLeds(bool lMenu, bool lBell, bool lPlus, bool lMinus);
void setBrightness(float dimmingValue);
void showBrightness(int brightness);

void setup() {
  /*Init shift register für die ansteuerung der LED's im Ring*/
  pinMode(PIN_DO_LED_Data, OUTPUT);
  digitalWrite(PIN_DO_LED_Data, LOW);
  pinMode(PIN_DO_LED_Clk, OUTPUT);
  digitalWrite(PIN_DO_LED_Clk, LOW);
  pinMode(PIN_DO_LED_Clear, OUTPUT);
  digitalWrite(PIN_DO_LED_Clear, HIGH);
  
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
  tft.init(135, 240);   //Init ST7789 240x135px
  tft.setSPISpeed(40000000);
  tft.fillScreen(ST77XX_BLACK);
  tft.setRotation(3);
  tft.setFont(&FreeSansBold24pt7b);
  tft.setTextColor(ST77XX_RED, ST77XX_GREEN);
  tft.setCursor(50,40);
  tft.print("maxon");
  tft.setFont(&FreeSansBold9pt7b);
  tft.setTextColor(ST77XX_WHITE, ST77XX_GREEN);
  tft.setCursor(50,70);
  tft.print("Berufsbildung");
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
  
  showTurningPoint(10);
  tft.fillScreen(ST77XX_BLACK);
  printTime();
  printDate();
  printTempHum(0.0, 0.0, ST77XX_BLACK);
  setButtonLeds(0,0,0,0);

}

void loop() {
  static int oldSecond = -1;
  static int oldMinute = -1;
  static int settingCounter = 0;
  static bool knobSettingPressed = false;
  static bool knobPressed = false;
  static float sensorValueTemperature = 0.0;
  static float sensorValueHuminity = 0.0;
  static float sensorValueBrightness= 0.0;
  static uint8_t brightnessDimmingValue;
  int actSecond = rtc.getSecond();
  int actMinute = rtc.getMinute();
  int SettingWaitCounter = 0;
  long newTime = 0 ;
  uint8_t inputValues = 0;
  if(oldSecond != actSecond){
    secondChanged = true;
    oldSecond = actSecond;
  }
  if(oldMinute != actMinute){
    minuteChanged = true;
    oldMinute = actMinute;
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

  //Check encoderknob is pressed for settings
  //if(pcf8574.digitalRead(BUTTON_UP) == PRESSED && knobSettingPressed == false){
  if(pcf8574.readButton(BUTTON_UP) == PRESSED && knobPressed == false){
    knobPressed = true;
    settingCounter++;
    Serial.print("UP_pressed Value: ");
    Serial.println(settingCounter);
  }
  if(pcf8574.readButton(BUTTON_DOWN) == PRESSED && knobPressed == false){
    knobPressed = true;
    settingCounter--;
    Serial.print("UP_pressed Value: ");
    Serial.println(settingCounter);
  }


  //delay(10);
  while (pcf8574.readButton(BUTTON_MENU) == PRESSED && state == STATE_TIMESET) { //wartezeit langes drücken Menu für Settingstart
    SettingWaitCounter++;
    if(SettingWaitCounter >= 100){
      knobSettingPressed = true;
      setButtonLeds();
      //alarmSetWaitCounter = 0;
      //tft.fillRect(230,2,5,SettingWaitCounter,ST77XX_WHITE);
      printSetting(0);
      break;
    }
    tft.fillRect(230,2,5,SettingWaitCounter,ST77XX_RED);
    Serial.println(SettingWaitCounter);
    delay(20);
  }
  if(SettingWaitCounter > 0 && state == STATE_TIMESET){  //Zurücksetzen des roten Wartebalkens
    tft.fillRect(230,2,5,SettingWaitCounter,ST77XX_WHITE);
  }
  if(pcf8574.readButton(BUTTON_MENU) == PRESSED && state != STATE_TIMESET){
    knobSettingPressed = true;
      setButtonLeds();
    delay(10);
  }
//}
  //delay(10);
  while (pcf8574.readButton(BUTTON_BELL) == PRESSED && state == STATE_TIMESET && knobPressed == false) { //state == STATE_TIMESET
    SettingWaitCounter++;
    if(SettingWaitCounter >= 50){
      knobPressed = true;
      //alarmSetWaitCounter = 0;
      alarmIsActive = !alarmIsActive;
      setButtonLeds();
      //tft.fillRect(230,2,5,SettingWaitCounter,ST77XX_WHITE);
      printAlarmTime();
      //printSetting(0);
      break;
    }
    tft.fillRect(230,2,5,SettingWaitCounter*2,ST77XX_RED);
    //Serial.println(SettingWaitCounter);
    delay(20);
  }
  

  switch (state) {
    case STATE_NOTIMESET:
      if(secondChanged){
        showActualTime(); 

        if((actSecond % 2) == 1){
          setBrightness(20.0);
        }else{
          setBrightness(sensorValueBrightness);
          showBrightness(sensorValueBrightness / 10);
        }
        if((actSecond % 10) == 0){
          printTempHum(sensorValueTemperature, sensorValueHuminity, ST77XX_BLACK);
        }
      }
      if(minuteChanged){printTime(); printTimeWarning();} //TODO: anpassen auf Minuten Changed
      //if(dayChanged){printDate(state, bufSecond);}
      break;
    case STATE_TIMESET:
      //if(secondChanged){printTime(state, bufSecond);} //TODO: anpassen auf Minuten Changed alarmOn
      if(secondChanged){
        showActualTime();
        if(!alarmOn){
          setBrightness(sensorValueBrightness);
        }else{
          if((actSecond % 2) == 1){
            setBrightness(20.0);
          }else{
            setBrightness(100.0);
          }
        }
        

        printTempHum(sensorValueTemperature, sensorValueHuminity, ST77XX_WHITE);
        showBrightness(sensorValueBrightness / 10);

      }
      if(minuteChanged){printTime();}
      break;
    case STATE_SETSETTINGMODE:
      if(setTimeOK){
        settingChapter = settingCounter % 5;
      }else{
        settingChapter = SETTING_CHAPTER_TIME;
      }
      printSetting(settingChapter);
      break;
    case STATE_TIMESETMIN:
      newTime = rtc.getLocalEpoch() + (settingCounter * 60);
      rtc.setTime(newTime);
	    //rtc_ext.SetTime(newTime);
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
      //timeSetOk = true;
      break;
    case STATE_ALARMSETMIN:
      alarmSetMin = alarmSetMin + settingCounter;
      if(alarmSetMin >=60){
        alarmSetMin = 0;
      }else if(alarmSetMin < 0){
        alarmSetMin = 59;
      }
      settingCounter = 0;
      printTime();
      break;
    case STATE_ALARMSETHR:
      alarmSetHr = alarmSetHr + settingCounter;
      if(alarmSetHr >=24){
        alarmSetHr = 0;
      }else if(alarmSetHr < 0){
        alarmSetHr = 23;
      }
      settingCounter = 0;
      printTime();
      break;
    case STATE_SETBRIGHTNESS:
      //displayBrightness = (newPosition % 9)*31+7;
      //setBackLight(displayBrightness, 1);
      //printBrightness();
      Serial.println("Brightnesssetting");
      break;
    default:
      // Statement(s)
      break; // Wird nicht benötigt, wenn Statement(s) vorhanden sind
  }

  //cange state for settings
  if(knobSettingPressed == true){
    //settingCounter = 0;
    switch (state) {
      case STATE_NOTIMESET:
        state = STATE_SETSETTINGMODE;
        break;
      case STATE_TIMESET:
        state = STATE_SETSETTINGMODE;
        break;
      case STATE_SETSETTINGMODE:
        tft.fillScreen(ST77XX_YELLOW);
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
          case SETTING_CHAPTER_DISPLAYBRIGHTNESS:
            state = STATE_SETBRIGHTNESS;
            break;
          case SETTING_CHAPTER_EXIT:
            if(setTimeOK == 0){
              state = STATE_NOTIMESET;
            }else{
              state = STATE_TIMESET;
            }
            tft.fillScreen(ST77XX_WHITE);
            printTime();
            printDate();
            //printAlarmTime();
            break;
          default:
            // Statement(s)
            break;
        }
        oldSettingChapter = -1;
        
        //state = STATE_TIMESETMIN;
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

      
      setButtonLeds(0,0,0,0);
      Serial.print(rtc.getDay());
      Serial.print(" - ");
      Serial.print(rtc.getMonth());
      Serial.print(" - ");
      Serial.println(rtc.getYear());
      
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
        setButtonLeds(0,0,0,0);
        setTimeOK = true;
        tft.fillScreen(ST77XX_WHITE);
        printTime();
        printDate();
        //printAlarmTime();
        break;
      case STATE_ALARMSETMIN:
        state = STATE_ALARMSETHR;
        break;
      case STATE_ALARMSETHR:
        alarmIsActive = 1;
        setButtonLeds(0,1,0,0);
        state = STATE_TIMESET;
        tft.fillScreen(ST77XX_WHITE);
        printTime();
        printDate();
        printAlarmTime();
        break;
      case STATE_SETBRIGHTNESS:
        //newPosition = 0;
        state = STATE_TIMESET;
        tft.fillScreen(ST77XX_WHITE);
        printTime();
        printDate();
        printAlarmTime();
        break;
      default:
        state = STATE_NOTIMESET;
        break; // Wird nicht benötigt, wenn Statement(s) vorhanden sind
        
    }
    delay(10);
  }
  delay(10);
  
  //inputValues = pcf8574.read8();
  //if(pcf8574.readButton(BUTTON_MENU) == RELEASED && knobSettingPressed == true  && state != STATE_SETSETTINGMODE){
  if(pcf8574.readButton(BUTTON_MENU) == RELEASED && knobSettingPressed == true){
    //tft.fillRect(230,2,5,100,ST77XX_WHITE); //löscht wartebalken
    knobSettingPressed = false;
    Serial.println("released");
  }
  //}
  if(pcf8574.readButton(BUTTON_BELL) == RELEASED && pcf8574.readButton(BUTTON_UP) == RELEASED && pcf8574.readButton(BUTTON_DOWN) == RELEASED && knobPressed == true){
    //tft.fillRect(230,2,5,100,ST77XX_WHITE); //löscht wartebalken
    knobPressed = false;
    Serial.println("released");
  }
  //}
//Serial.println(state);

  //Check is time for alarm and alarm activated
  if(alarmSetMin == rtc.getMinute() && alarmSetHr == rtc.getHour(true) && alarmIsActive && state == STATE_TIMESET){
    alarmOn = true;
    rtc_ext.SetGPOState(false);
    Serial.println("Alarm Start");
    alarmIsActive = 0;
  }

  //Check is alarmbutton presst for stop alarm
  if(pcf8574.readButton(BUTTON_BELL) == PRESSED && alarmOn == true){
    alarmOn = 0;
    rtc_ext.SetGPOState(true);
    Serial.println("Alarm Stop");
  }
  
  secondChanged = false;
  minuteChanged = false;
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


/*
*Zeigt eine Show dar auf dem LED Ring (Punkt der im Kreis dreht)
*/
void showTurningPoint(int speed){
  digitalWrite(PIN_DO_LED_Clear, LOW);
  delay(1);
  digitalWrite(PIN_DO_LED_Clear, HIGH);
  delay(1);
  digitalWrite(PIN_DO_LED_Data, HIGH);
  delay(1);
  digitalWrite(PIN_DO_LED_Clk, HIGH);
  delay(1);
  digitalWrite(PIN_DO_LED_Clk, LOW);
  delay(1);
  digitalWrite(PIN_DO_LED_Data, LOW);
  delay(1);
  for(int x=0; x<=179; x++){
    digitalWrite(PIN_DO_LED_Clk, HIGH);
    delay(1);
    digitalWrite(PIN_DO_LED_Clk, LOW);
    delay(speed);
  }
}

/*
*Stellt die aktuelle Zeit auf dem LED Ring dar
*/
void showActualTime(){
  bool timeArray[3][60]{0};
  int positionSecond = 59 - rtc.getSecond();
  int positionMinute = 59 - rtc.getMinute();
  int positionHour = 59 - ((rtc.getHour(false) * 5) + rtc.getMinute() / 12);
  if(positionHour < 0){positionHour = 59 + positionHour;}  //Korrektur bei 12Uhr

  timeArray[redArray][positionHour] = 1;
  timeArray[redArray][positionHour+1] = 1;
  timeArray[blueArray][positionMinute] = 1;
  timeArray[greenArray][positionSecond] = 1;
  digitalWrite(PIN_DO_LED_Clear, LOW);
  delay(1);
  digitalWrite(PIN_DO_LED_Clear, HIGH);
  delay(1);
  for(int x=0; x<3; x++){
    for(int y=0; y<60; y++){
      digitalWrite(PIN_DO_LED_Data, timeArray[x][y]);
      //delayMicroseconds(2);
      digitalWrite(PIN_DO_LED_Clk, HIGH);
      //delayMicroseconds(2);
      digitalWrite(PIN_DO_LED_Clk, LOW);
      //delayMicroseconds(2);
    }
  }
}

/*
*Schreibt die aktuelle Zeit und das Datum auf das Display
*/
/*void printTime(){
  char timeString[6] = " ";
  rtc.getTime("%H:%M").toCharArray(timeString, 7);
  rtc.getTime("%d.%m.%Y").toCharArray(dateString, 11);
  tft.fillScreen(ST77XX_BLACK);
  tft.setFont(&FreeSansBold24pt7b);
  tft.setTextColor(ST77XX_BLUE, ST77XX_GREEN);
  tft.setCursor(60,40);
  tft.print(timeString);
  tft.setCursor(2,90);
  tft.print(dateString);
}*/

/**
 * @brief Schreibt die aktuelle Zeit und das Datum auf das Display
 */
void printTime(){
  char timeString[6] = " ";
  char dateString[11] = " ";
  //tft.fillRect(70,15,120,40,ST77XX_YELLOW);
  //tft.setCursor(70, 50);
  tft.setFont(&FreeSansBold24pt7b);
  tft.setTextWrap(true);
  //tft.print("22:32");
  switch (state) {
  case STATE_NOTIMESET:
      rtc.getTime("%H:%M").toCharArray(timeString, 7);
      rtc.getTime("%d.%m.%Y").toCharArray(dateString, 11);  
      tft.setTextColor(ST77XX_RED, ST77XX_GREEN);
      tft.fillRect(60,5,120,40,ST77XX_YELLOW);
      tft.setCursor(60,40);
      tft.print(timeString);
      //tft.setCursor(2,90);
      //tft.print(dateString);
    break;
  case STATE_TIMESET:
      rtc.getTime("%H:%M").toCharArray(timeString, 7);
      rtc.getTime("%d.%m.%Y").toCharArray(dateString, 11);
      tft.setTextColor(ST77XX_RED, ST77XX_GREEN);
      tft.fillRect(60,5,120,40,ST77XX_WHITE);
      tft.setCursor(60,40);
      tft.print(timeString);
      //tft.setCursor(2,90);
      //tft.print(dateString);
    break;
  case STATE_TIMESETMIN:
      //setBackLight(255, 1);
      tft.fillRect(125,5,50,40,ST77XX_CYAN);
      rtc.getTime("%H:%M").toCharArray(timeString, 7);
      tft.setCursor(60,40);
      tft.print(timeString);
      showActualTime();
    break;
  case STATE_TIMESETHR:
      //setBackLight(255, 1);
      tft.fillRect(60,5,50,40,ST77XX_CYAN);
      rtc.getTime("%H:%M").toCharArray(timeString, 7);
      tft.setCursor(60,40);
      tft.print(timeString);
      showActualTime();
    break;
  case STATE_ALARMSETMIN:
      //setBackLight(255, 1);
      tft.fillRect(125,5,50,40,ST77XX_CYAN);
      sprintf(timeString, "%02d:%02d", alarmSetHr, alarmSetMin);
      tft.setCursor(60,40);
      tft.print(timeString);
    break;
  case STATE_ALARMSETHR:
      //setBackLight(255, 1);
      tft.fillRect(60,5,50,40,ST77XX_CYAN);
      sprintf(timeString, "%02d:%02d", alarmSetHr, alarmSetMin);
      tft.setCursor(60,40);
      tft.print(timeString);
    break;
  default:
    // Statement(s)
    break; // Wird nicht benötigt, wenn Statement(s) vorhanden sind
  }

  //blinkState = changeBlinkstate(blinkState);
  //tft.print(timeString);
  //Digiclock.setString(displayString);
}

void printDate(){
  char dateString[11] = " ";

  //tft.fillRect(60,15,140,40,ST77XX_BLUE);
  //tft.fillRect(40,64,170,32,ST77XX_YELLOW);
  
  tft.setFont(&FreeSansBold18pt7b);
  tft.setCursor(40, 80);
  tft.setTextWrap(true);
  //tft.print("22:32");
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
  rtc.getTime("%d.%m.%Y").toCharArray(dateString, 11);
  tft.print(dateString);
}

void printAlarmTime(){ //TODO: Anpassen mit Texten zum ausgeben
  char alarmString[16] = " ";
  sprintf(alarmString, "Weckzeit: %02d:%02d", alarmSetHr, alarmSetMin);
  tft.setFont(&FreeSansBold9pt7b);
  tft.fillRect(20,119,186,22,ST77XX_WHITE);
  tft.setCursor(20, 131);
  if(alarmIsActive){
    tft.fillRect(20,119,186,22,ST77XX_YELLOW);
    //tft.print("Weckzeit:");
    tft.print(alarmString);
  }else{
    tft.fillRect(20,119,186,22,ST77XX_WHITE);
    tft.print("Wecker aus");
  }
}

void printTempHum(float temperature, float humidity, uint16_t bgcolor){ //TODO: Anpassen mit Texten zum ausgeben
  tft.setFont(&FreeSansBold18pt7b);
  tft.fillRect(0,115,110,-25, bgcolor);
  tft.setCursor(5, 115);
  tft.print(temperature, 1);
  tft.print("*C");
  
  tft.fillRect(120,115,115,-25, bgcolor);
  tft.setCursor(120, 115);
  tft.print(humidity, 1);
  tft.print(" %");
}

void printSetting(int pointPlace){ //TODO: Anpassen mit Texten zum ausgeben
  if(oldSettingChapter != settingChapter){
  oldSettingChapter = settingChapter;
  char displayString[6] = "";
  //setBackLight(255, 1);
  tft.fillScreen(ST77XX_YELLOW);
  tft.setFont(&FreeSansBold18pt7b);
  tft.setCursor(10, 50);
  tft.setTextWrap(true);
  tft.print("Settings");
  tft.setCursor(10, 90);
  switch (pointPlace) {
    case SETTING_CHAPTER_TIME:
      tft.print("Zeit/Datum"); 
      break;
    case SETTING_CHAPTER_ALARMTIME:
      tft.print("Weckzeit");
      break;
    case SETTING_CHAPTER_REFRESHRATE:
      tft.print("akt. Sensor");
      break;
    case SETTING_CHAPTER_DISPLAYBRIGHTNESS:
      tft.print("Beleuchtung");
      break;
    case SETTING_CHAPTER_EXIT:
      tft.print("Exit");
      break;
    default:
      tft.print("Settings");
      break; // Wird nicht benötigt, wenn Statement(s) vorhanden sind
    }
  //Digiclock.setString(displayString);
  }
}

void printTimeWarning(){ //TODO: Anpassen mit Texten zum ausgeben
  tft.setFont(&FreeSansBold18pt7b);
  tft.fillRect(0,30,30,-30,ST77XX_YELLOW);
  tft.setCursor(9, 27);
  tft.print("!");
  tft.setFont(&FreeSansBold9pt7b);
  tft.setCursor(20, 131);
  tft.print("Zeit nicht eingestellt!!");
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
  tft.fillRect(235,30,5, 100-brightness,ST77XX_WHITE);
  tft.fillRect(235,130,5, -brightness,ST77XX_ORANGE);
}
