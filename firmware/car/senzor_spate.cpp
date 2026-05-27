// ============================================================
//  senzor_spate.cpp
//  HC-SR04 montat in spatele masinii
//
//  Conexiuni:
//    VCC  -> 5V
//    GND  -> GND
//    ECHO -> D4 (PD4)
//    TRIG -> D5 (PD5)
// ============================================================

#include <avr/io.h>
#include <util/delay.h>
#include "Arduino.h"      // necesar pentru pulseIn() si Serial
#include "senzor_spate.h"

// --- Registri si masti pentru pini ---
// ECHO: D4 = PD4
#define ECHO_DDR   DDRD
#define ECHO_PORT  PORTD
#define ECHO_PIN   PIND
#define ECHO_BIT   PD4

// TRIG: D5 = PD5
#define TRIG_DDR   DDRD
#define TRIG_PORT  PORTD
#define TRIG_BIT   PD5

// --- Praguri distanta (cm) ---
static const int DIST_PERICOL = 15;
static const int DIST_ATENTIE = 30;

static float _ultimaDistanta = 999.0;

// ============================================================
namespace SenzorSpate {

    void init() {
        // ECHO (PD4) -> INPUT (stergem bitul din DDR)
        ECHO_DDR &= ~(1 << ECHO_BIT);
        // Dezactivam pull-up intern pe ECHO
        ECHO_PORT &= ~(1 << ECHO_BIT);

        // TRIG (PD5) -> OUTPUT
        TRIG_DDR |= (1 << TRIG_BIT);

        // TRIG initial LOW
        TRIG_PORT &= ~(1 << TRIG_BIT);

        Serial.println("[SenzorSpate] initializat pe registri (Echo=D4/PD4, Trig=D5/PD5)");
    }

    float citeste() {
        // --- Trimite puls TRIG de 10 µs ---
        TRIG_PORT &= ~(1 << TRIG_BIT);
        _delay_us(2);

        TRIG_PORT |= (1 << TRIG_BIT);
        _delay_us(10);

        TRIG_PORT &= ~(1 << TRIG_BIT);

        // --- Asteapta ecoul pe ECHO (PD4) cu timeout 25ms ~ 4m ---
        long durata = pulseIn(4, HIGH, 25000);  // pin 4 = PD4

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
