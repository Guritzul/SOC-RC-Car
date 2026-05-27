#pragma once

#define JOY_L_PIN A1
#define JOY_L_SW 4

void joystickLeftInit();
int joystickLeftThrottle(); // 0=înapoi, 1023=înainte
bool joystickLeftPressed();