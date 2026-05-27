#pragma once

#define BTN_BUZZ 6
#define BUZZER 7

void buzzerInit();
bool buzzerButtonPressed();
void buzzerUpdate(bool active);