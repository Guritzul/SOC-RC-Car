#include "buzzer.h"
#include <Arduino.h>

void buzzerInit()
{
    pinMode(BTN_BUZZ, INPUT_PULLUP);
    pinMode(BUZZER, OUTPUT);
}

bool buzzerButtonPressed()
{
    return !digitalRead(BTN_BUZZ);
}

void buzzerUpdate(bool active)
{
    if (active)
        tone(BUZZER, 1000);
    else
        noTone(BUZZER);
}