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
    return !_buttonGpio.read();
}

void Buzzer::update(bool active)
{
    if (active)
    {
        tone(_buzzerPinNum, 1000);
    }
    else if (_isBeepingOnce)
    {
        if (millis() - _beepStartTime >= 100) {
            _isBeepingOnce = false;
            noTone(_buzzerPinNum);
        } else {
            tone(_buzzerPinNum, 1500);
        }
    }
    else
    {
        noTone(_buzzerPinNum);
    }
}

void Buzzer::beepOnce()
{
    _isBeepingOnce = true;
    _beepStartTime = millis();
}