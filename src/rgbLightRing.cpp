#include "rgbLightRing.h"

RgbLedRing::RgbLedRing(uint8_t pinData, uint8_t pinClk, uint8_t pinClear) {
  _pinData = pinData;  // Pin Data Anschluss
  _pinClk = pinClk;  // Pin Clock Anschluss
  _pinClear = pinClear;  // Pin Clear Anschluss
}


/*
*initialisierung der Pins
*/
void RgbLedRing::begin() {
  pinMode(_pinData, OUTPUT);
  digitalWrite(_pinData, LOW);
  pinMode(_pinClk, OUTPUT);
  digitalWrite(_pinClk, LOW);
  pinMode(_pinClear, OUTPUT);
  digitalWrite(_pinClear, HIGH);
}

/*
*löscht alle LED's
*/
void RgbLedRing::clearRing() {
  digitalWrite(_pinClear, LOW);
  delay(delaySignalChange);
  digitalWrite(_pinClear, HIGH);
  delay(delaySignalChange);
}

/*
*setzt einen neuen Punkt
*/
void RgbLedRing::setPoint() {
  digitalWrite(_pinData, HIGH);
  delay(delaySignalChange);
  digitalWrite(_pinClk, HIGH);
  delay(delaySignalChange);
  digitalWrite(_pinClk, LOW);
  delay(delaySignalChange);
  digitalWrite(_pinData, LOW);
  delay(delaySignalChange);
}

/*
*schiebt den Punkt eine stelle weiter
*/
void RgbLedRing::shiftPoint() {
    digitalWrite(_pinClk, HIGH);
    delay(delaySignalChange);
    digitalWrite(_pinClk, LOW);
    delay(delaySignalChange);
}

/*
*Zeigt eine Show dar auf dem LED Ring (Punkt der im Kreis dreht)
*/
void RgbLedRing::showTurningPoint(int speed){
  clearRing();
  setPoint();
  for(int x=0; x<=179; x++){
    shiftPoint();
    delay(speed);
  }
}

/*
*Zeigt eine Show dar auf dem LED Ring (Punkt der im Kreis dreht)
*/
void RgbLedRing::showTurningPointWhite(int speed){
  clearRing();
  setPoint();
  for(int x=0; x<=58; x++){
    shiftPoint();
    delay(speed);
  }
  setPoint();
  for(int x=0; x<=58; x++){
    shiftPoint();
    delay(speed);
  }
  setPoint();
  for(int x=0; x<=58; x++){
    shiftPoint();
    delay(speed);
  }
}

void RgbLedRing::showTurningWorm(int speed){
  clearRing();
  setPoint();
    delay(speed);
  shiftPoint();
    delay(speed);
  setPoint();
    delay(speed);
  shiftPoint();
    delay(speed);
  setPoint();
    delay(speed);
  shiftPoint();
    delay(speed);
  for(int x=0; x<=180; x++){
    shiftPoint();
    delay(speed);
  }
}