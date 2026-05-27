#include "joystick_left.h"
#include <Arduino.h>

void joystickLeftInit()
{
    pinMode(JOY_L_SW, INPUT_PULLUP);
}

int joystickLeftThrottle()
{
    return 1023 - analogRead(JOY_L_PIN);
}

bool joystickLeftPressed()
{
    return !digitalRead(JOY_L_SW);
}