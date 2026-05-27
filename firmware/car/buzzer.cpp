// ============================================================
//  buzzer.cpp
//  Alerta sonora in functie de proximitate obstacol
//
//  Conexiuni:
//    (+) -> A3 (PC3)
//    (-) -> GND
// ============================================================

#include <avr/io.h>
#include "Arduino.h"    // necesar pentru tone(), noTone(), millis()
#include "buzzer.h"

// --- Pin Buzzer: A3 = PC3 = Arduino analog pin 3 ---
// tone() / noTone() folosesc numarul pinului Arduino
static const int PIN_BUZZER = A3;   // A3 = pin analog 3 (PC3)

// --- Timpi pentru bip non-blocking ---
static unsigned long _ultimulBip  = 0;
static bool          _buzzerActiv = false;

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
        // PC3 (A3) -> OUTPUT pe registri
        DDRC |= (1 << DDC3);
        // Initial LOW (buzzer stins)
        PORTC &= ~(1 << PC3);

        noTone(PIN_BUZZER);
        _buzzerActiv = false;

        Serial.println("[Buzzer] initializat pe registri (Pin=A3/PC3)");
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
