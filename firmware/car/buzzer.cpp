// ============================================================
//  buzzer.cpp
//  Alerta sonora in functie de proximitate obstacol
//
//  Conexiuni:
//    (+) -> Pin 8
//    (-) -> GND
// ============================================================

#include "Arduino.h"
#include "buzzer.h"

static const int PIN_BUZZER = 8;

// Timpi pentru bip non-blocking
static unsigned long _ultimulBip    = 0;
static bool          _buzzerActiv   = false;

// Intervale bip (ms)
static const int INTERVAL_PERICOL = 150;   // bip rapid
static const int INTERVAL_ATENTIE = 500;   // bip lent
static const int DURATA_BIP       = 100;   // cat tine un bip

// Frecvente ton
static const int FREQ_PERICOL = 1000;
static const int FREQ_ATENTIE = 600;

// ============================================================
namespace Buzzer {

    void init() {
        pinMode(PIN_BUZZER, OUTPUT);
        noTone(PIN_BUZZER);
        Serial.println("[Buzzer] initializat (Pin=8)");
    }

    void liniste() {
        noTone(PIN_BUZZER);
        _buzzerActiv = false;
    }

    void bipAtentie() {
        tone(PIN_BUZZER, FREQ_ATENTIE, DURATA_BIP);
    }

    void bipPericol() {
        tone(PIN_BUZZER, FREQ_PERICOL, DURATA_BIP);
    }

    // Non-blocking: apelat o data per loop()
    // Gestioneaza ritmul bipurilor automat
    void update(bool pericol, bool atentie) {
        unsigned long acum = millis();

        if (!pericol && !atentie) {
            liniste();
            return;
        }

        int interval = pericol ? INTERVAL_PERICOL : INTERVAL_ATENTIE;

        if (acum - _ultimulBip >= (unsigned long)interval) {
            _ultimulBip = acum;
            if (pericol) {
                bipPericol();
            } else {
                bipAtentie();
            }
        }
    }

} // namespace Buzzer
