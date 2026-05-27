// ============================================================
//  senzor_fata.cpp
//  HC-SR04 montat in fata masinii
//
//  Conexiuni:
//    VCC  -> 5V
//    GND  -> GND
//    TRIG -> Pin 2
//    ECHO -> Pin 3
// ============================================================

#include "Arduino.h"
#include "senzor_fata.h"

// --- Pini ---
static const int TRIG = 2;
static const int ECHO = 3;

// --- Praguri distanta (cm) ---
static const int DIST_PERICOL = 15;
static const int DIST_ATENTIE = 30;

// Ultima distanta masurata (cache intre apeluri)
static float _ultimaDistanta = 999.0;

// ============================================================
namespace SenzorFata {

    void init() {
        pinMode(TRIG, OUTPUT);
        pinMode(ECHO, INPUT);
        digitalWrite(TRIG, LOW);
        Serial.println("[SenzorFata] initializat (Trig=2, Echo=3)");
    }

    float citeste() {
        // Trimite puls de 10 µs
        digitalWrite(TRIG, LOW);
        delayMicroseconds(2);
        digitalWrite(TRIG, HIGH);
        delayMicroseconds(10);
        digitalWrite(TRIG, LOW);

        // Asteapta ecoul (timeout 25ms ~ 4m)
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

} // namespace SenzorFata
