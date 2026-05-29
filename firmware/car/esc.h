#pragma once

// Conexiuni: Semnal PWM -> D10 (PB2 / OC1B), GND comun
// Observatie: Timer 1 este partajat cu Steering (OC1A pe D9); ESC foloseste canalul OC1B.
namespace Esc {
    void init();
    void setThrottle(int rawValue);
    void stop();
}
