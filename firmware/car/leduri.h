#pragma once

// ============================================================
//  leduri.h
//  Control LED-uri masina RC:
//    - Far fata    -> A1  (PC1, digital on/off)
//    - Stop spate  -> Pin 6 (PD6/OC0A, PWM Timer 0)
// ============================================================

namespace Leduri {

    void init();

    // Far fata
    void farFataOn();
    void farFataOff();

    // Stop spate
    void stopNormal();    // aprins slab (mers)
    void stopFrana();     // aprins puternic (frana)
    void stopOff();       // stins
}