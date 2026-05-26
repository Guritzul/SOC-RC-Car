#pragma once

// ============================================================
//  senzor_lumina.h
//  HW-072 - aprinde farurile automat cand e intuneric
//
//  Conexiuni:
//    VCC -> 5V
//    GND -> GND
//    AO  -> A0
//    DO  -> neconectat
// ============================================================

namespace SenzorLumina {
    void init();
    int citeste();        // returneaza 0 (lumina) - 1023 (intuneric)
    bool esteIntuneric(); // true cand farurile trebuie aprinse
}
