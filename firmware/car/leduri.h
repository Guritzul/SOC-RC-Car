#pragma once

// ============================================================
//  leduri.h
//  Control LED-uri masina RC:
//    - Far fata    (on/off)
//    - Stop spate  (on/off + luminozitate frana)
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