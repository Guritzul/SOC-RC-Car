// Conexiuni: AO -> A0 (PC0 / ADC0)
#include <avr/io.h>
#include <Arduino.h>
#include "senzor_lumina.h"

// Prag: peste aceasta valoare = intuneric -> faruri ON (0 = lumina maxima, 1023 = intuneric total)
static const int PRAG_INTUNERIC = 600;
static int _ultimaValoare = 0;

namespace SenzorLumina {

    void init() {
        DDRC &= ~(1 << DDC0);
        PORTC &= ~(1 << PC0);

        // ADMUX: REFS0=1 (AVcc ca referinta), ADLAR=0 (dreapta), MUX3:0=0000 (canal ADC0)
        ADMUX = (1 << REFS0);

        // ADCSRA: ADEN=1 (activare ADC), Prescaler = 128 (16 MHz / 128 = 125 kHz clock ADC)
        ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);

        // Prima conversie de incalzire (se ignora - ADC are nevoie de un ciclu initial)
        ADCSRA |= (1 << ADSC);
        while (ADCSRA & (1 << ADSC)) {}

        Serial.println("[SenzorLumina] initializat pe registri ADC (AO=A0/PC0/ADC0)");
    }

    int citeste() {
        ADMUX = (1 << REFS0);
        ADCSRA |= (1 << ADSC);

        while (ADCSRA & (1 << ADSC)) {}

        _ultimaValoare = (int)ADC;
        return _ultimaValoare;
    }

    bool esteIntuneric() {
        return _ultimaValoare > PRAG_INTUNERIC;
    }

} // namespace SenzorLumina
