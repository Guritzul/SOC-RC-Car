#include "joystick_right.h"
#include <Arduino.h>

void joystickRightInit()
{
    pinMode(JOY_R_SW, INPUT_PULLUP);
}

int joystickRightSteering()
{
    return 1023 - analogRead(JOY_R_PIN);
}

bool joystickRightPressed()
{
    return !digitalRead(JOY_R_SW);
}