// ============================================================
//  senzor_fata.cpp
//  HC-SR04 montat in fata masinii
//
//  Conexiuni:
//    VCC  -> 5V
//    GND  -> GND
//    ECHO -> D2 (PD2)
//    TRIG -> D3 (PD3)
// ============================================================

#include <avr/io.h>
#include <util/delay.h>
#include "Arduino.h"      // necesar pentru pulseIn() si Serial
#include "senzor_fata.h"

// --- Registri si masti pentru pini ---
// ECHO: D2 = PD2
#define ECHO_DDR   DDRD
#define ECHO_PORT  PORTD
#define ECHO_PIN   PIND
#define ECHO_BIT   PD2

// TRIG: D3 = PD3
#define TRIG_DDR   DDRD
#define TRIG_PORT  PORTD
#define TRIG_BIT   PD3

// --- Praguri distanta (cm) ---
static const int DIST_PERICOL = 15;
static const int DIST_ATENTIE = 30;

// Ultima distanta masurata (cache intre apeluri)
static float _ultimaDistanta = 999.0;

// ============================================================
namespace SenzorFata {

    void init() {
        // ECHO (PD2) -> INPUT (stergem bitul corespunzator din DDR)
        ECHO_DDR &= ~(1 << ECHO_BIT);
        // Dezactivam pull-up intern pe ECHO
        ECHO_PORT &= ~(1 << ECHO_BIT);

        // TRIG (PD3) -> OUTPUT
        TRIG_DDR |= (1 << TRIG_BIT);

        // TRIG initial LOW
        TRIG_PORT &= ~(1 << TRIG_BIT);

        Serial.println("[SenzorFata] initializat pe registri (Echo=D2/PD2, Trig=D3/PD3)");
    }

    float citeste() {
        // --- Trimite puls TRIG de 10 µs ---
        // LOW pentru curatare
        TRIG_PORT &= ~(1 << TRIG_BIT);
        _delay_us(2);

        // HIGH timp de 10 µs
        TRIG_PORT |= (1 << TRIG_BIT);
        _delay_us(10);

        // Revenire LOW
        TRIG_PORT &= ~(1 << TRIG_BIT);

        // --- Asteapta ecoul pe ECHO (PD2) cu timeout 25ms ~ 4m ---
        // pulseIn() este folosit pentru masurarea precisa a duratei pulsului
        // (foloseste timer hardware intern Arduino - nu este acces la pin Arduino HAL)
        long durata = pulseIn(2, HIGH, 25000);  // pin 2 = PD2

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
