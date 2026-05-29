#pragma once

#include "hal.h"

class Joystick
{
private:
    IAdc &_adc;
    IGpio &_switchGpio;
    uint8_t _adcChannel;
    bool _invertAxis;

public:
    Joystick(IAdc &adc, IGpio &switchGpio, uint8_t adcChannel, bool invertAxis = false);

    void init();
    uint16_t readAxis();
    bool isPressed();
};
