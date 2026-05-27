// ============================================================
//  senzor_lumina.cpp
//  HW-072 - aprinde farurile automat cand e intuneric
//
//  Conexiuni:
//    VCC -> 5V
//    GND -> GND
//    AO  -> A0 (PC0 / ADC0)
//    DO  -> neconectat
// ============================================================

#include <avr/io.h>
#include <Arduino.h>
#include "senzor_lumina.h"

// --- Registri ADC ---
// A0 = ADC0 (PC0) -> canal 0 in MUX-ul ADMUX

// Prag: peste aceasta valoare = intuneric -> faruri ON
// 0 = lumina maxima, 1023 = intuneric total
static const int PRAG_INTUNERIC = 600;

static int _ultimaValoare = 0;

// ============================================================
namespace SenzorLumina {

    void init() {
        // PC0 (A0) -> INPUT (implicit la reset, dar setam explicit)
        DDRC &= ~(1 << DDC0);
        // Dezactivam pull-up intern
        PORTC &= ~(1 << PC0);

        // Initializare ADC:
        // ADMUX: REFS1=0, REFS0=1 -> AVcc ca referinta
        //        ADLAR=0 -> aliniere dreapta (10 biti in ADC)
        //        MUX3:0=0000 -> canal ADC0
        ADMUX = (1 << REFS0);

        // ADCSRA: ADEN=1 (activare ADC)
        //         Prescaler = 128 (ADPS2=1, ADPS1=1, ADPS0=1)
        //         16 MHz / 128 = 125 kHz clock ADC (optim pentru 10-bit)
        ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);

        // Prima conversie de incalzire (se ignora - ADC are nevoie de un ciclu initial)
        ADCSRA |= (1 << ADSC);
        while (ADCSRA & (1 << ADSC)) { /* asteapta */ }

        Serial.println("[SenzorLumina] initializat pe registri ADC (AO=A0/PC0/ADC0)");
    }

    int citeste() {
        // Selectam canal ADC0 (MUX = 0000), referinta AVcc
        ADMUX = (1 << REFS0);  // canal 0, REFS0=1 (AVcc)

        // Declansam conversia
        ADCSRA |= (1 << ADSC);

        // Asteptam finalizarea (ADSC se sterge automat de hardware)
        while (ADCSRA & (1 << ADSC)) { /* busy-wait */ }

        // Citim rezultatul pe 10 biti (ADCL mai intai, apoi ADCH - automizat de hardware)
        _ultimaValoare = (int)ADC;  // registrul ADC combina automat ADCL + ADCH

        return _ultimaValoare;
    }

    bool esteIntuneric() {
        return _ultimaValoare > PRAG_INTUNERIC;
    }

} // namespace SenzorLumina
