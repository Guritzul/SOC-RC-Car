// Conexiuni: ECHO -> D2 (PD2), TRIG -> D3 (PD3)
#include <avr/io.h>
#include <util/delay.h>
#include "Arduino.h"
#include "senzor_fata.h"

#define ECHO_DDR   DDRD
#define ECHO_PORT  PORTD
#define ECHO_PIN   PIND
#define ECHO_BIT   PD2

#define TRIG_DDR   DDRD
#define TRIG_PORT  PORTD
#define TRIG_BIT   PD3

static const int DIST_PERICOL = 20;
static const int DIST_ATENTIE = 30;

static float _ultimaDistanta = 999.0;

namespace SenzorFata {

    void init() {
        ECHO_DDR &= ~(1 << ECHO_BIT);
        ECHO_PORT &= ~(1 << ECHO_BIT);

        TRIG_DDR |= (1 << TRIG_BIT);
        TRIG_PORT &= ~(1 << TRIG_BIT);

        Serial.println("[SenzorFata] initializat pe registri (Echo=D2/PD2, Trig=D3/PD3)");
    }

    float citeste() {
        TRIG_PORT &= ~(1 << TRIG_BIT);
        _delay_us(2);

        TRIG_PORT |= (1 << TRIG_BIT);
        _delay_us(10);

        TRIG_PORT &= ~(1 << TRIG_BIT);

        // pulseIn() măsoară durata ecoului cu un timeout de 25ms (~ 4m)
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
