// ============================================================
//  senzor_lumina.cpp
//  HW-072 - aprinde farurile automat cand e intuneric
//
//  Conexiuni:
//    VCC -> 5V
//    GND -> GND
//    AO  -> A0
//    DO  -> neconectat
// ============================================================

#include "Arduino.h"
#include "senzor_lumina.h"

static const int PIN_AO = A0;

// Prag: peste aceasta valoare = intuneric -> faruri ON
// 0 = lumina maxima, 1023 = intuneric total
// Ajusteaza daca farurile se aprind prea devreme/tarziu
static const int PRAG_INTUNERIC = 600;

static int _ultimaValoare = 0;

// ============================================================
namespace SenzorLumina {

    void init() {
        pinMode(PIN_AO, INPUT);
        Serial.println("[SenzorLumina] initializat (AO=A0)");
    }

    int citeste() {
        _ultimaValoare = analogRead(PIN_AO);
        return _ultimaValoare;
    }

    bool esteIntuneric() {
        return _ultimaValoare > PRAG_INTUNERIC;
    }

} // namespace SenzorLumina
