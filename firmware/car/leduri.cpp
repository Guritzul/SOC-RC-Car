// ============================================================
//  leduri.cpp
//  Control LED-uri masina RC
//
//  Conexiuni (rezistenta 220Ω in serie cu fiecare LED):
//    Far fata    -> Pin 9  (digital on/off)
//    Stop spate  -> Pin 10 (PWM - luminozitate variabila)
// ============================================================

#include "Arduino.h"
#include "leduri.h"

static const int PIN_FAR_FATA   = 9;
static const int PIN_STOP_SPATE = 10;  // PWM

static const int PWM_STOP_NORMAL = 60;   // ~25% - lumina de pozitie
static const int PWM_STOP_FRANA  = 255;  // 100% - frana activa

// ============================================================
namespace Leduri {

    void init() {
        pinMode(PIN_FAR_FATA,   OUTPUT);
        pinMode(PIN_STOP_SPATE, OUTPUT);

        digitalWrite(PIN_FAR_FATA,  LOW);
        analogWrite(PIN_STOP_SPATE, 0);

        Serial.println("[Leduri] initializat (Far=9, Stop=10)");
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