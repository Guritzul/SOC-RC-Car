#include "buzzer.h"
#include <Arduino.h>

Buzzer::Buzzer(IGpio &buttonGpio, IGpio &buzzerGpio, uint8_t buzzerPinNum)
    : _buttonGpio(buttonGpio), _buzzerGpio(buzzerGpio), _buzzerPinNum(buzzerPinNum)
{
}

void Buzzer::init()
{
    _buttonGpio.initInputPullup();
    _buzzerGpio.initOutput();
}

bool Buzzer::isButtonPressed()
{
    return !_buttonGpio.read(); // Button is active LOW due to pull-up
}

void Buzzer::update(bool active)
{
    if (active)
    {
        tone(_buzzerPinNum, 1000);
    }
    else
    {
        noTone(_buzzerPinNum);
    }
}