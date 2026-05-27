#include "hal.h"
#include "joystick.h"
#include "buzzer.h"
#include "radio_tx.h"

// ============================================================
//  Static Component Instantiation and Dependency Injection (SOLID)
// ============================================================

// 1. Initialize the global ADC engine
static Atm328Adc adcEngine;

// 2. Instantiate Left Joystick (Throttle axis + push button)
//    - Analog Pin: A1 (ATmega328P Channel 1)
//    - Digital Pin: D4 (PD4)
//    - Axis Direction: Inverted (matching hardware direction mapping)
static Atm328Gpio leftJoystickSwitch(&DDRD, &PORTD, &PIND, PORTD4);
static Joystick leftJoystick(adcEngine, leftJoystickSwitch, 1, true);

// 3. Instantiate Right Joystick (Steering axis + push button)
//    - Analog Pin: A2 (ATmega328P Channel 2)
//    - Digital Pin: D5 (PD5)
//    - Axis Direction: Inverted (matching hardware direction mapping)
static Atm328Gpio rightJoystickSwitch(&DDRD, &PORTD, &PIND, PORTD5);
static Joystick rightJoystick(adcEngine, rightJoystickSwitch, 2, true);

// 4. Instantiate Buzzer
//    - Button Pin: D6 (PD6)
//    - Speaker Pin: D7 (PD7)
static Atm328Gpio buzzerButtonPin(&DDRD, &PORTD, &PIND, PORTD6);
static Atm328Gpio buzzerSpeakerPin(&DDRD, &PORTD, &PIND, PORTD7);
static Buzzer remoteBuzzer(buzzerButtonPin, buzzerSpeakerPin, 7);

void setup()
{
  Serial.begin(9600);

  // Initialize ADC
  adcEngine.init();

  // Initialize joysticks and buzzer
  leftJoystick.init();
  rightJoystick.init();
  remoteBuzzer.init();

  // Initialize nRF24L01 radio transmitter using register-level SOLID driver
  RadioTx::init();

  Serial.println("TX Ready");
}

void loop()
{
  // 1. Read input state from joysticks and buttons
  Payload data;
  data.throttle = leftJoystick.readAxis();
  data.steering = rightJoystick.readAxis();
  data.buzz = remoteBuzzer.isButtonPressed();
  data.swLeft = leftJoystick.isPressed();
  data.swRight = rightJoystick.isPressed();

  // 2. Update buzzer audio feedback before transmitting
  remoteBuzzer.update(data.buzz);

  // 3. Send data package over the air via nRF24L01 transmitter
  bool success = RadioTx::send(data);

  // 4. Output debug diagnostics to the Serial console
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