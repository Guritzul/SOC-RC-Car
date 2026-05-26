#define JOY_THR   A0
#define JOY_STR   A3
#define JOY_L_SW  4
#define JOY_R_SW  5
#define BTN_BUZZ  6
#define BUZZER    7

void setup() {
  Serial.begin(9600);
  pinMode(JOY_L_SW, INPUT_PULLUP);
  pinMode(JOY_R_SW, INPUT_PULLUP);
  pinMode(BTN_BUZZ, INPUT_PULLUP);
  pinMode(BUZZER, OUTPUT);
}

void loop() {
  int throttle = 1023 - analogRead(JOY_THR);
  int steering = 1023 - analogRead(JOY_STR);

  bool buzz = !digitalRead(BTN_BUZZ);

  if (buzz) tone(BUZZER, 1000);
  else noTone(BUZZER);

  Serial.print("Throttle="); Serial.print(throttle);
  Serial.print("  Steering="); Serial.print(steering);
  Serial.print("  BUZZ="); Serial.println(buzz);

  delay(100);
}
