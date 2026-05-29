#include "hal.h"
#include "joystick.h"
#include "buzzer.h"
#include "radio_tx.h"

static Atm328Adc adcEngine;

// Stânga: A1, buton D4 (PD4)
static Atm328Gpio leftJoystickSwitch(&DDRD, &PORTD, &PIND, PORTD4);
static Joystick leftJoystick(adcEngine, leftJoystickSwitch, 1, true);

// Dreapta: A2, buton D5 (PD5)
static Atm328Gpio rightJoystickSwitch(&DDRD, &PORTD, &PIND, PORTD5);
static Joystick rightJoystick(adcEngine, rightJoystickSwitch, 2, true);

// Buzzer: buton D6 (PD6), speaker D3 (PD3)
static Atm328Gpio buzzerButtonPin(&DDRD, &PORTD, &PIND, PORTD6);
static Atm328Gpio buzzerSpeakerPin(&DDRD, &PORTD, &PIND, PORTD3);
static Buzzer remoteBuzzer(buzzerButtonPin, buzzerSpeakerPin, 3);

void setup()
{
  Serial.begin(9600);

  adcEngine.init();
  leftJoystick.init();
  rightJoystick.init();
  remoteBuzzer.init();
  RadioTx::init();

  Serial.println("TX Ready");
}

void loop()
{
  static bool lastSwLeft = false;
  static bool lastSwRight = false;

  Payload data;
  data.throttle = leftJoystick.readAxis();
  data.steering = rightJoystick.readAxis();
  data.buzz = remoteBuzzer.isButtonPressed();
  data.swLeft = leftJoystick.isPressed();
  data.swRight = rightJoystick.isPressed();

  // Detectare schimbare treaptă de viteză
  if ((data.swLeft && !lastSwLeft) || (data.swRight && !lastSwRight)) {
      remoteBuzzer.beepOnce();
  }
  lastSwLeft = data.swLeft;
  lastSwRight = data.swRight;

  remoteBuzzer.update(data.buzz);

  bool success = RadioTx::send(data);

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
  Serial.println(success ? "OK" : "FAIL");

  delay(50);
}