#ifndef RGB_LIGHT_RING_H
#define RGB_LIGHT_RING_H

#include <Arduino.h>

class RgbLedRing {
  public:
    /**
     * Konstruktor
     * @param pinData Anschluss Schieberegister Data Pin
     * @param pinClk Anschluss Schieberegister Clock Pin
     * @param pinClear Anschluss Schieberegister Clear Pin
     */
    RgbLedRing(uint8_t pinData, uint8_t pinClk, uint8_t pinClear);  // Pin wird beim Erstellen übergeben
    void begin();
    void clearRing();
    void showTurningPoint(int speed);
    void showTurningPointWhite(int speed);
    void showTurningWorm(int speed);

  private:
    uint8_t _pinData;   // Pin Data Anschluss
    uint8_t _pinClk;    // Pin Clock Anschluss
    uint8_t _pinClear;  // Pin Clear Anschluss

    const uint32_t delaySignalChange = 1; 

    void setPoint();
    void shiftPoint();
};

#endif