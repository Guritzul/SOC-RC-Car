// Conexiuni: (+) -> A3 (PC3), (-) -> GND
#include <avr/io.h>
#include "Arduino.h"
#include "buzzer.h"

static const int PIN_BUZZER = A3;

static unsigned long _ultimulBip  = 0;
static bool          _buzzerActiv = false;

static const int INTERVAL_PERICOL = 150;
static const int INTERVAL_ATENTIE = 500;
static const int DURATA_BIP       = 100;

static const int FREQ_PERICOL = 1000;
static const int FREQ_ATENTIE = 600;

namespace Buzzer {

    void init() {
        DDRC |= (1 << DDC3);
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

    void update(bool pericol, bool atentie, bool claxon, bool marsarier, float distSpate) {
        if (claxon) {
            tone(PIN_BUZZER, FREQ_PERICOL);
            return;
        }

        unsigned long acum = millis();
        int interval = 0;
        int freq = FREQ_ATENTIE;

        if (marsarier) {
            // Asistenta parcare: frecventa creste proportional (interval scade)
            interval = (int)distSpate * 15;
            if (interval > 1000) interval = 1000;
            if (interval < 150) interval = 150;
            freq = 800; // Ton distinct pentru marsarier
        } else if (pericol) {
            interval = INTERVAL_PERICOL;
            freq = FREQ_PERICOL;
        } else if (atentie) {
            interval = INTERVAL_ATENTIE;
            freq = FREQ_ATENTIE;
        }

        if (interval == 0) {
            liniste();
            return;
        }

        if (acum - _ultimulBip >= (unsigned long)interval) {
            _ultimulBip = acum;
            tone(PIN_BUZZER, freq, DURATA_BIP);
        }
    }

} // namespace Buzzer
