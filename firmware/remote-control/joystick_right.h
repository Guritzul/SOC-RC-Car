#pragma once

#define JOY_R_PIN A3
#define JOY_R_SW 5

void joystickRightInit();
int joystickRightSteering(); // 0=stânga, 1023=dreapta
bool joystickRightPressed();