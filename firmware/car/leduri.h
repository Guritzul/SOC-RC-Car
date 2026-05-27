#pragma once

// ============================================================
//  leduri.h
//  Control LED-uri masina RC:
//    - Far fata   -> A1 (PC1, digital on/off)
//    - Stop spate -> A2 (PC2, PWM software prin update())
// ============================================================

namespace Leduri {

    void init();

    // Far fata (digital)
    void farFataOn();
    void farFataOff();

    // Stop spate - seteaza starea dorita
    void stopNormal();    // aprins slab ~25% (mers) - PWM software
    void stopFrana();     // aprins puternic 100% (frana)
    void stopOff();       // stins

    // Apelat din loop() pentru a gestiona PWM software Stop Spate
    void update();
}