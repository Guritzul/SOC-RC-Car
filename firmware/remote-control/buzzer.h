#pragma once

#include "hal.h"

// C++ Buzzer component managing a button input pin and a buzzer output pin
class Buzzer
{
private:
    IGpio &_buttonGpio;
    IGpio &_buzzerGpio;
    uint8_t _buzzerPinNum; // Retained for Arduino's tone() compatibility

public:
    Buzzer(IGpio &buttonGpio, IGpio &buzzerGpio, uint8_t buzzerPinNum);

    // Initialise the buzzer and button pins via HAL
    void init();

    // Returns true if the buzzer activation button is pressed
    bool isButtonPressed();

    // Updates the buzzer sound state (toggles 1 kHz tone)
    void update(bool active);
};