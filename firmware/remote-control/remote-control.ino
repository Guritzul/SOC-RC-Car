#include "joystick_left.h"
#include "joystick_right.h"
#include "buzzer.h"
#include "antenna.h"

void setup()
{
  Serial.begin(9600);
  joystickLeftInit();
  joystickRightInit();
  buzzerInit();
  antennaInit();
  Serial.println("TX ready");
}

void loop()
{
  Payload data;
  data.throttle = joystickLeftThrottle();
  data.steering = joystickRightSteering();
  data.buzz = buzzerButtonPressed();
  data.swLeft = joystickLeftPressed();
  data.swRight = joystickRightPressed();

  buzzerUpdate(data.buzz);

  bool ok = antennaSend(data);

  Serial.print("THR=");
  Serial.print(data.throttle);
  Serial.print(" STR=");
  Serial.print(data.steering);
  Serial.print(" BUZ=");
  Serial.print(data.buzz);
  Serial.print(" SWL=");
  Serial.print(data.swLeft);
  Serial.print(" SWR=");
  Serial.print(data.swRight);
  Serial.print(" TX=");
  Serial.println(ok ? "OK" : "FAIL");

  delay(50);
}