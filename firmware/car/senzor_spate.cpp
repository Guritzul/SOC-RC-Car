// ============================================================
//  senzor_spate.cpp
//  HC-SR04 montat in spatele masinii
//
//  Conexiuni:
//    VCC  -> 5V
//    GND  -> GND
//    TRIG -> Pin 4
//    ECHO -> Pin 5
// ============================================================

#include "Arduino.h"
#include "senzor_spate.h"

// --- Pini ---
static const int TRIG = 4;
static const int ECHO = 5;

// --- Praguri distanta (cm) ---
static const int DIST_PERICOL = 15;
static const int DIST_ATENTIE = 30;

static float _ultimaDistanta = 999.0;

// ============================================================
namespace SenzorSpate {

    void init() {
        pinMode(TRIG, OUTPUT);
        pinMode(ECHO, INPUT);
        digitalWrite(TRIG, LOW);
        Serial.println("[SenzorSpate] initializat (Trig=4, Echo=5)");
    }

    float citeste() {
        digitalWrite(TRIG, LOW);
        delayMicroseconds(2);
        digitalWrite(TRIG, HIGH);
        delayMicroseconds(10);
        digitalWrite(TRIG, LOW);

        long durata = pulseIn(ECHO, HIGH, 25000);

        if (durata == 0) {
            _ultimaDistanta = 999.0;
        } else {
            _ultimaDistanta = (durata * 0.0343f) / 2.0f;
        }

        return _ultimaDistanta;
    }

    bool estePericol() {
        return _ultimaDistanta < DIST_PERICOL;
    }

    bool esteAtentie() {
        return _ultimaDistanta < DIST_ATENTIE;
    }

} // namespace SenzorSpate
