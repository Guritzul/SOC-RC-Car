#pragma once

// ============================================================
//  buzzer.h
//  Alerta sonora in functie de proximitate obstacol
// ============================================================

namespace Buzzer {
    void init();
    void liniste();
    void bipAtentie();     // obstacol in zona de atentie (30cm)
    void bipPericol();     // obstacol in zona de pericol (15cm)
    void update(bool pericol, bool atentie);  // apelat din loop()
}
