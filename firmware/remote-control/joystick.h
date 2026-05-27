#pragma once

#include "hal.h"

// Unified C++ Joystick component managing an analog axis and a digital switch button
class Joystick
{
private:
    IAdc &_adc;
    IGpio &_switchGpio;
    uint8_t _adcChannel;
    bool _invertAxis;

public:
    Joystick(IAdc &adc, IGpio &switchGpio, uint8_t adcChannel, bool invertAxis = false);

    // Initialise the joystick GPIO pins (switch as input pullup)
    void init();
    
    // Reads and returns the analog axis value (0-1023)
    uint16_t readAxis();

    // Returns true if the joystick button is pressed
    bool isPressed();
};
