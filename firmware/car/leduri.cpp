// ============================================================
//  leduri.cpp
//  Control LED-uri masina RC
//
//  Conexiuni (rezistenta 220Ω in serie cu fiecare LED):
//    Far fata   -> A1 (PC1, digital on/off)
//    Stop spate -> A2 (PC2, digital on/off + PWM software in update())
// ============================================================

#include <avr/io.h>
#include "Arduino.h"    // necesar pentru millis() in PWM software
#include "leduri.h"

// --- Registri si masti pentru pini (Port C) ---
// Far fata:   A1 = PC1
#define FAR_FATA_BIT    PC1
// Stop spate: A2 = PC2
#define STOP_SPATE_BIT  PC2

// --- Parametri PWM software pentru Stop Spate ---
// Simuleaza luminozitate redusa prin ON/OFF rapid
// Ciclu PWM: PERIOD_MS ms total, LED aprins DUTY_ON_MS ms
static const unsigned int PWM_PERIOD_MS  = 20;    // frecventa ~50 Hz
static const unsigned int PWM_DUTY_ON_MS = 5;     // ~25% duty cycle (lumina slaba)

// Starea dorita pentru stop spate
typedef enum {
    STOP_OFF    = 0,
    STOP_NORMAL = 1,   // PWM software ~25%
    STOP_FRANA  = 2    // 100% ON
} StopState;

static StopState _stopState = STOP_OFF;
static unsigned long _pwmLastTick = 0;
static bool _pwmPhase = false;  // false = OFF phase, true = ON phase

// ============================================================
namespace Leduri {

    void init() {
        // PC1 (A1) -> OUTPUT
        DDRC |= (1 << FAR_FATA_BIT);
        // PC2 (A2) -> OUTPUT
        DDRC |= (1 << STOP_SPATE_BIT);

        // Stingere initiala
        PORTC &= ~(1 << FAR_FATA_BIT);
        PORTC &= ~(1 << STOP_SPATE_BIT);

        _stopState = STOP_OFF;
        _pwmLastTick = 0;
        _pwmPhase = false;

        Serial.println("[Leduri] initializat pe registri (Far=A1/PC1, Stop=A2/PC2)");
    }

    // --- Far fata (digital) ---

    void farFataOn() {
        PORTC |= (1 << FAR_FATA_BIT);
    }

    void farFataOff() {
        PORTC &= ~(1 << FAR_FATA_BIT);
    }

    // --- Stop spate: seteaza starea dorita (efectul e aplicat in update()) ---

    void stopNormal() {
        _stopState = STOP_NORMAL;
    }

    void stopFrana() {
        _stopState = STOP_FRANA;
        // Aprinde imediat pentru reactie rapida
        PORTC |= (1 << STOP_SPATE_BIT);
    }

    void stopOff() {
        _stopState = STOP_OFF;
        PORTC &= ~(1 << STOP_SPATE_BIT);
    }

    // --- update(): apelat din loop() - gestioneaza PWM software pentru Stop Normal ---
    // Trebuie apelat cat mai des (cel putin la fiecare 5ms)
    void update() {
        if (_stopState == STOP_OFF) {
            PORTC &= ~(1 << STOP_SPATE_BIT);
            return;
        }

        if (_stopState == STOP_FRANA) {
            PORTC |= (1 << STOP_SPATE_BIT);
            return;
        }

        // STOP_NORMAL: PWM software 25% duty cycle
        unsigned long acum = millis();
        unsigned long elapsed = acum - _pwmLastTick;

        if (!_pwmPhase) {
            // Faza OFF: asteptam (PERIOD - DUTY_ON) ms
            if (elapsed >= (PWM_PERIOD_MS - PWM_DUTY_ON_MS)) {
                _pwmPhase = true;
                _pwmLastTick = acum;
                PORTC |= (1 << STOP_SPATE_BIT);  // aprinde
            }
        } else {
            // Faza ON: asteptam DUTY_ON ms
            if (elapsed >= PWM_DUTY_ON_MS) {
                _pwmPhase = false;
                _pwmLastTick = acum;
                PORTC &= ~(1 << STOP_SPATE_BIT);  // stinge
            }
        }
    }

} // namespace Leduri