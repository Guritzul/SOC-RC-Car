#pragma once

// ============================================================
//  senzor_spate.h
//  HC-SR04 montat in spatele masinii
// ============================================================

namespace SenzorSpate {
    void init();
    float citeste();       // returneaza distanta in cm (999 = liber)
    bool estePericol();    // < DIST_PERICOL
    bool esteAtentie();    // < DIST_ATENTIE
}
