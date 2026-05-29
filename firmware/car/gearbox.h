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

    void init();
    void setGear(Gear gear);
    void shiftUp();
    void shiftDown();
    Gear getGear();
}
