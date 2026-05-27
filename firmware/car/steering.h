#pragma once

namespace Steering
{
    // Inițializează servodirecția (Timer 1 pe registri, frecvență 50Hz, periodă 20ms)
    void init();

    // Setează unghiul roților primind valoarea brută din joystick (0 - 1023, 512 = centru)
    void setAngle(int rawValue);
}
