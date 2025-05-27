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
#define BUTTON_ENTER 5
#define BUTTON_UP 6
#define BUTTON_DOWN 7
#define PRESSED false
#define RELEASED true

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
int state = STATE_NOTIMESET;
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
void printTimeWarning();
void printTempHum(float temperatutre, float humidity, uint16_t bgcolor);
//void calculateBrightness(uint8_t *calculatetDimmingValue);
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

  /*Init Lichtsensor*/
  veml.begin();

  /*Init Echtzeituhr*/
  rtc_ext.SetAlarm1Enabled(false);
  rtc_ext.SetAlarm2Enabled(false);
  rtc_ext.SetSquareWaveOutputState(false);
  rtc_ext.SetGPOState(false);
  rtc_ext.SetOscillatorEnabled(true);
  rtc_ext.SetBatteryEnabled(true);
  rtc_ext.Set24HourTimeEnabled(true);
  rtc_ext.HasPowerFailed(&timeInternFailDown, &timeInternFailUp);
  rtc_ext.ClearPowerFailedFlag();

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

  delay(500);
  rtc_ext.SetGPOState(true);

  delay(1500);

  Serial.println("Start");

  rtc_ext.ReadTime(&timeIntern);
  exttimeToEsptime();
  exttimePrintToSerial();
  
  showTurningPoint(10);
  tft.fillScreen(ST77XX_BLACK);
  printTime();
  printDate();

}

void loop() {
  static int oldSecond = -1;
  static int oldMinute = -1;
  static int settingCounter = 0;
  static bool knobPressed = false;
  static bool knobSettingPressed = false;
  static float sensorValueTemperature = 0.0;
  static float sensorValueHuminity = 0.0;
  static float sensorValueBrightness= 0.0;
  static uint8_t brightnessDimmingValue;
  int actSecond = rtc.getSecond();
  int actMinute = rtc.getMinute();
  int SetingWaitCounter = 0;
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
        Serial.print(sensorValueTemperature);
        Serial.print(" ; ");
        Serial.print(sensorValueHuminity);
        Serial.print(" ; ");
        Serial.print(sensorValueBrightness);
        Serial.println(" ; ");
        exttimePrintToSerial();
  }

  //Check encoderknob is pressed for settings
  //if(pcf8574.digitalRead(BUTTON_UP) == PRESSED && knobPressed == false){
  if(pcf8574.readButton(BUTTON_UP) == PRESSED && knobSettingPressed == false){
    knobSettingPressed = true;
    settingCounter++;
    Serial.print("UP_pressed Value: ");
    Serial.println(settingCounter);
  }
  if(pcf8574.readButton(BUTTON_DOWN) == PRESSED && knobSettingPressed == false){
    knobSettingPressed = true;
    settingCounter--;
    Serial.print("UP_pressed Value: ");
    Serial.println(settingCounter);
  }


  //delay(10);
  while (pcf8574.readButton(BUTTON_MENU) == PRESSED && state == STATE_TIMESET) { //state == STATE_TIMESET
    SetingWaitCounter++;
    if(SetingWaitCounter >= 100){
      knobPressed = true;
      //alarmSetWaitCounter = 0;
      //printSetting(0);
    }
    tft.fillRect(216,2,2,SetingWaitCounter,ST77XX_RED);
    Serial.println(SetingWaitCounter);
    delay(20);
  }
  if(pcf8574.readButton(BUTTON_MENU) == PRESSED && state != STATE_TIMESET){
    knobPressed = true;
    delay(10);
  }
//}

  
  /*if(pcf8574.digitalRead(BUTTON_UP) == RELEASED && knobPressed == true){
    knobPressed = false;
    Serial.println("UPreleased");
  }*/

  switch (state) {
    case STATE_NOTIMESET:
      //if(secondChanged){printTime(state, bufSecond);} //TODO: anpassen auf Minuten Changed
      if(secondChanged){
        showActualTime(); 

        if((actSecond % 2) == 1){
          setBrightness(20.0);
        }else{
          setBrightness(sensorValueBrightness);
          showBrightness(sensorValueBrightness / 10);
        }
        if((actSecond % 5) == 0){
          printTempHum(sensorValueTemperature, sensorValueHuminity, ST77XX_BLACK);
        }
      }
      if(minuteChanged){printTime(); printTimeWarning();} //TODO: anpassen auf Minuten Changed
      //if(dayChanged){printDate(state, bufSecond);}
      break;
    case STATE_TIMESET:
      //if(secondChanged){printTime(state, bufSecond);} //TODO: anpassen auf Minuten Changed
      if(secondChanged){
        showActualTime();
        setBrightness(sensorValueBrightness);
        printTempHum(sensorValueTemperature, sensorValueHuminity, ST77XX_WHITE);
        showBrightness(sensorValueBrightness / 10);

      }
      if(minuteChanged){printTime();}
      break;
    case STATE_SETSETTINGMODE:
      /*if(newPosition < 0){
        encoder.setCount(6);
        newPosition = 3;
      }
      if(timeSetOk){
        settingChapter = newPosition % 5;
      }else{
        settingChapter = SETTING_CHAPTER_TIME;
      }
      printSetting(settingChapter);*/
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
    /*case STATE_ALARMSETMIN:
      alarmSetMin = alarmSetMin + newPosition;
      if(alarmSetMin >=60){
        alarmSetMin = 0;
      }else if(alarmSetMin < 0){
        alarmSetMin = 59;
      }
      encoder.setCount(0);
      printTime(state, bufSecond);
      break;
    case STATE_ALARMSETHR:
      alarmSetHr = alarmSetHr + newPosition;
      if(alarmSetHr >=24){
        alarmSetHr = 0;
      }else if(alarmSetHr < 0){
        alarmSetHr = 23;
      }
      encoder.setCount(0);
      printTime(state, bufSecond);
      break;
    case STATE_SETBRIGHTNESS:
      displayBrightness = (newPosition % 9)*31+7;
      setBackLight(displayBrightness, 1);
      printBrightness();
      break;*/
    default:
      // Statement(s)
      break; // Wird nicht benötigt, wenn Statement(s) vorhanden sind
  }

   //Check encoderknob is pressed for settings
  if(pcf8574.readButton(BUTTON_MENU) == PRESSED && knobPressed == false){
    //if( knobPressed == false){
    
    Serial.println("pressed");
    

    delay(10);
    /*while (pcf8574.readButton(BUTTON_MENU) == PRESSED && state == STATE_TIMESET) { //state == STATE_TIMESET
      SetingWaitCounter++;
      Serial.println("pressed");
      if(SetingWaitCounter >= 10){
        knobPressed = true;
        settingCounter = 0;
        
        Serial.println("pressedloop");
        //alarmSetWaitCounter = 0;
        //printSetting(0);
      }
      tft.fillRect(220,30,20,SetingWaitCounter,ST77XX_RED);
      Serial.println(SetingWaitCounter);
      delay(10);
    }
    if(pcf8574.readButton(BUTTON_MENU) == PRESSED && state != STATE_TIMESET){
      knobPressed = true;
      settingCounter = 0;
      delay(10);
    }*/
  }
  
  //tft.fillRect(220,30,20,100,ST77XX_BLACK);

    //cange state for settings
    if(knobPressed == true){
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
          /*switch (settingChapter) {
            case SETTING_CHAPTER_TIME:*/
              state = STATE_TIMESETMIN;        
              printTime();
              printDate();
              /*break;
            case SETTING_CHAPTER_ALARMTIME:
              state = STATE_ALARMSETMIN;
              break;
            case SETTING_CHAPTER_DISPLAYBRIGHTNESS:
              state = STATE_SETBRIGHTNESS;
              break;
            case SETTING_CHAPTER_EXIT:
              if(timeSetOk == 0){
                state = STATE_NOTIMESET;
              }else{
                state = STATE_TIMESET;
              }
              tft.fillScreen(ST77XX_WHITE);
              printTime(state, bufSecond);
              printDate(state, bufSecond);
              printAlarmTime();
              break;
            default:
              // Statement(s)
              break;
          }
          oldSettingChapter = -1;*/
          
          state = STATE_TIMESETMIN;
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
          
          //timeIntern.minone = 5;
          //timeIntern.minten = 2;
          rtc_ext.SetTime(&timeIntern);

          state = STATE_TIMESET;
          tft.fillScreen(ST77XX_WHITE);
          printTime();
          printDate();
          //printAlarmTime();
          break;
        /*case STATE_ALARMSETMIN:
          state = STATE_ALARMSETHR;
          break;
        case STATE_ALARMSETHR:
          state = STATE_TIMESET;
          tft.fillScreen(ST77XX_WHITE);
          printTime(state, bufSecond);
          printDate(state, bufSecond);
          printAlarmTime();
          break;
        case STATE_SETBRIGHTNESS:
          newPosition = 0;
          if(timeSetOk == 0){
            state = STATE_NOTIMESET;
          }else{
            state = STATE_TIMESET;
          }
          tft.fillScreen(ST77XX_WHITE);
          printTime(state, bufSecond);
          printDate(state, bufSecond);
          printAlarmTime();
          break;*/
        default:
          state = STATE_NOTIMESET;
          break; // Wird nicht benötigt, wenn Statement(s) vorhanden sind
          
      }
      delay(10);
    }
    delay(10);
    
    //inputValues = pcf8574.read8();
    if(pcf8574.readButton(BUTTON_MENU) == RELEASED && knobPressed == true  && state != STATE_SETSETTINGMODE){
      knobPressed = false;
      Serial.println("released");
    }
    //}
    if(pcf8574.readButton(BUTTON_ENTER) == RELEASED && pcf8574.readButton(BUTTON_UP) == RELEASED && pcf8574.readButton(BUTTON_DOWN) == RELEASED && knobSettingPressed == true){
      knobSettingPressed = false;
      Serial.println("released");
    }
    //}
  //Serial.println(state);
  
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
      tft.fillRect(130,10,50,40,ST77XX_CYAN);
      rtc.getTime("%H:%M").toCharArray(timeString, 7);
      tft.setCursor(60,40);
      tft.print(timeString);
      showActualTime();
    break;
  case STATE_TIMESETHR:
      //setBackLight(255, 1);
      tft.fillRect(70,10,50,40,ST77XX_CYAN);
      rtc.getTime("%H:%M").toCharArray(timeString, 7);
      tft.setCursor(60,40);
      tft.print(timeString);
      showActualTime();
    break;
  /*case STATE_ALARMSETMIN:
      setBackLight(255, 1);
      tft.fillRect(130,15,50,40,ST77XX_CYAN);
      sprintf(timeString, "%02d:%02d", alarmSetHr, alarmSetMin);
    break;
  case STATE_ALARMSETHR:
      setBackLight(255, 1);
      tft.fillRect(70,15,50,40,ST77XX_CYAN);
      sprintf(timeString, "%02d:%02d", alarmSetHr, alarmSetMin);
    break;*/
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

void printTempHum(float temperature, float humidity, uint16_t bgcolor){ //TODO: Anpassen mit Texten zum ausgeben
  tft.setFont(&FreeSansBold18pt7b);
  tft.fillRect(0,115,110,-25, bgcolor);
  tft.setCursor(5, 115);
  tft.print(temperature, 1);
  tft.print(" °C");
  
  tft.fillRect(120,115,115,-25, bgcolor);
  tft.setCursor(120, 115);
  tft.print(humidity, 1);
  tft.print(" %");
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
