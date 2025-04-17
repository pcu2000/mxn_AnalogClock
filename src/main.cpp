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
#include <Fonts/FreeSansBold24pt7b.h>

/*Pin definitions*/

/*LED's*/
#define PIN_DO_LED_Data 4
#define PIN_DO_LED_Clk 2
#define PIN_DO_LED_Clear 3
#define PIN_PWM_LED_Pwm 1
const int redArray = 2;
const int blueArray = 1;
const int greenArray = 0;

/*I2C*/
#define PIN_I2C_SDA 5
#define PIN_I2C_SCL 6
PCF8574 pcf8574(0x38);

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

/*Time definition*/
ESP32Time rtc(0);
int oldSecond = -1;
int oldMinute = -1;

void showTurningPoint(int speed);
void showActualTime();
void printTime();

void setup() {
  /*Init shift register for LED's*/
  pinMode(PIN_DO_LED_Data, OUTPUT);
  digitalWrite(PIN_DO_LED_Data, LOW);
  pinMode(PIN_DO_LED_Clk, OUTPUT);
  digitalWrite(PIN_DO_LED_Clk, LOW);
  pinMode(PIN_DO_LED_Clear, OUTPUT);
  digitalWrite(PIN_DO_LED_Clear, HIGH);
  
  /**TODO LED mit ESP32 PWM Funktion funktioniert noch nicht deshalb momentan mit AnalogWrite**/
  pinMode(PIN_PWM_LED_Pwm, OUTPUT);
  analogWrite(PIN_PWM_LED_Pwm, 10);
  //ledcSetup(PWM_CHANNEL_LED, PWM_FREQ, PWM_RESOLUTION);
  //ledcAttachPin(PIN_PWM_LED_Pwm, PWM_CHANNEL_LED);
  //ledcWrite(PIN_PWM_LED_Pwm, 255);

  /*Serial(USB) for debug*/
  Serial.begin(115200);
  //Serial.begin(9600);

  /*I2C*/
  /*Portexpander*/
	pcf8574.pinMode(P0, OUTPUT, HIGH);
	pcf8574.pinMode(P1, OUTPUT, LOW);
	pcf8574.pinMode(P2, OUTPUT, LOW);
	pcf8574.pinMode(P3, OUTPUT, LOW);

	pcf8574.pinMode(P7, INPUT);
	pcf8574.pinMode(P6, INPUT);
	pcf8574.pinMode(P5, INPUT);
	pcf8574.pinMode(P4, INPUT);

	//Serial.print("Init pcf8574...");
	if (pcf8574.begin()){
		//Serial.println("OK");
	}else{
		//Serial.println("failed");
	}

  /*Init Time*/
  rtc.setTime(47, 11, 20, 17, 4, 2025);

  /*Init Display*/
  tft.init(135, 240);   //Init ST7789 240x135px
  tft.setSPISpeed(40000000);
  tft.fillScreen(ST77XX_BLACK);
  tft.setRotation(3);
  tft.setFont(&FreeSansBold24pt7b);
  tft.setTextColor(ST77XX_BLUE, ST77XX_GREEN);
  tft.setCursor(10,40);
  tft.print("16:14:32");

  showTurningPoint(10);
}

void loop() {
  int actSecond = rtc.getSecond();
  int actMinute = rtc.getMinute();
  if(oldSecond != actSecond){
    oldSecond = actSecond;
    showActualTime();
    if((actSecond % 2) == 1){pcf8574.digitalWrite(P0, HIGH);}else{pcf8574.digitalWrite(P0, LOW);}
  }
  if(oldMinute != actMinute){
    oldMinute = actMinute;
    printTime();
  }
  delay(10);
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
void printTime(){
  char timeString[6] = " ";
  char dateString[11] = " ";
  rtc.getTime("%H:%M").toCharArray(timeString, 7);
  rtc.getTime("%d.%m.%Y").toCharArray(dateString, 11);
  tft.fillScreen(ST77XX_BLACK);
  tft.setRotation(3);
  tft.setFont(&FreeSansBold24pt7b);
  tft.setTextColor(ST77XX_BLUE, ST77XX_GREEN);
  tft.setCursor(60,40);
  tft.print(timeString);
  tft.setCursor(2,90);
  tft.print(dateString);
}
