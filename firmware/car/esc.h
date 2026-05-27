#pragma once

// ============================================================
//  esc.h
//  Driver ESC (Electronic Speed Controller) pentru motorul DC
//
//  Conexiuni:
//    Semnal PWM -> D10 (PB2 / OC1B)
//    Alimentare -> 6V-8.4V (din baterie, nu din Arduino)
//    GND        -> GND comun cu Arduino
//
//  Observatie: Timer 1 este partajat cu Steering (OC1A pe D9).
//  ESC-ul foloseste canalul OC1B al aceluiasi timer.
//  Initializarea Timer 1 se face o singura data in Steering::init().
//  Esc::init() activeaza doar OC1B (COM1B1) si seteaza pozitia neutra.
// ============================================================

namespace Esc {

    // Initializeaza ESC: activeaza OC1B pe Timer 1 si armeaza ESC-ul
    // (semnalul neutru de 1.5ms timp de 3 secunde pentru armare)
    void init();

    // Seteaza viteza motorului din valoarea bruta a joystick-ului (0-1023)
    //   0   -> inapoi maxim (daca ESC suporta reverse)
    //   512 -> stop (semnal neutru 1.5ms)
    //   1023-> inainte maxim
    void setThrottle(int rawValue);

    // Opreste motorul imediat (echivalent cu setThrottle(512))
    void stop();
}
