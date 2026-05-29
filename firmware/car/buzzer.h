#pragma once

// Pin: (+) -> A3 (PC3)
namespace Buzzer {
    void init();
    void liniste();
    void bipAtentie();
    void bipPericol();
    void update(bool pericol, bool atentie, bool claxon, bool marsarier, float distSpate);
}
