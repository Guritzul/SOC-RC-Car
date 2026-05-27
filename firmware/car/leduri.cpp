// ============================================================
//  leduri.cpp
//  Control LED-uri masina RC
//
//  Conexiuni (rezistenta 220Ω in serie cu fiecare LED):
//    Far fata    -> A1  (PC1, digital on/off)      [MUTAT de pe Pin 9 - conflict Timer1/Steering]
//    Stop spate  -> Pin 6 (PD6/OC0A, PWM Timer 0) [MUTAT de pe Pin 10 - conflict SPI CSN nRF24L01]
// ============================================================

#include "Arduino.h"
#include "leduri.h"

static const int PIN_FAR_FATA   = A1;   // PC1 - digital only, fara conflict
static const int PIN_STOP_SPATE = 6;    // PD6 / OC0A - PWM real pe Timer 0 (liber)

static const int PWM_STOP_NORMAL = 60;   // ~25% - lumina de pozitie
static const int PWM_STOP_FRANA  = 255;  // 100% - frana activa

// ============================================================
namespace Leduri {

    void init() {
        pinMode(PIN_FAR_FATA,   OUTPUT);
        pinMode(PIN_STOP_SPATE, OUTPUT);

        digitalWrite(PIN_FAR_FATA,  LOW);
        analogWrite(PIN_STOP_SPATE, 0);

        Serial.println("[Leduri] initializat (Far=A1, Stop=6)");
    }

    void farFataOn() {
        digitalWrite(PIN_FAR_FATA, HIGH);
    }

    void farFataOff() {
        digitalWrite(PIN_FAR_FATA, LOW);
    }

    void stopNormal() {
        analogWrite(PIN_STOP_SPATE, PWM_STOP_NORMAL);
    }

    void stopFrana() {
        analogWrite(PIN_STOP_SPATE, PWM_STOP_FRANA);
    }

    void stopOff() {
        analogWrite(PIN_STOP_SPATE, 0);
    }

} // namespace Leduri