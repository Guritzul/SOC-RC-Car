#pragma once

#include <stdint.h>

namespace Gearbox
{
    enum Gear
    {
        GEAR_1 = 1,
        GEAR_2 = 2,
        GEAR_3 = 3
    };

    // Initializează modulul gearbox: configurează pinul D6 (PD6) ca ieșire și
    // activează întreruperile Timer 1 overflow + Timer 2 Compare A pentru soft-PWM.
    void init();

    // Setează treapta de viteză direct
    void setGear(Gear gear);

    // Mărește treapta de viteză (1 -> 2 -> 3)
    void shiftUp();

    // Micșorează treapta de viteză (3 -> 2 -> 1)
    void shiftDown();

    // Returnează treapta de viteză curentă
    Gear getGear();
}
