#pragma once

#include "hal.h"

class Buzzer
{
private:
    IGpio &_buttonGpio;
    IGpio &_buzzerGpio;
    uint8_t _buzzerPinNum;

public:
    Buzzer(IGpio &buttonGpio, IGpio &buzzerGpio, uint8_t buzzerPinNum);

    void init();
    bool isButtonPressed();
    void update(bool active);
    void beepOnce();

private:
    unsigned long _beepStartTime = 0;
    bool _isBeepingOnce = false;
};