#pragma once

// ============================================================
//  senzor_fata.h
//  HC-SR04 montat in fata masinii
// ============================================================

namespace SenzorFata {
    void init();
    float citeste();       // returneaza distanta in cm (999 = liber)
    bool estePericol();    // < DIST_PERICOL
    bool esteAtentie();    // < DIST_ATENTIE
}
