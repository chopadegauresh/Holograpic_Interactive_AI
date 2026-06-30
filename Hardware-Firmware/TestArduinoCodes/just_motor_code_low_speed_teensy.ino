#include <Arduino.h>

#define RPWM_PIN 6
#define LPWM_PIN 7
#define REN_PIN  8
#define LEN_PIN  9

int motorSpeed = 200;   // target speed

void motorForward(int speed) {
  analogWrite(RPWM_PIN, speed);
  analogWrite(LPWM_PIN, 0);
}

void setup() {
  Serial.begin(115200);

  pinMode(RPWM_PIN, OUTPUT);
  pinMode(LPWM_PIN, OUTPUT);
  pinMode(REN_PIN, OUTPUT);
  pinMode(LEN_PIN, OUTPUT);

  digitalWrite(REN_PIN, HIGH);
  digitalWrite(LEN_PIN, HIGH);

  analogWriteFrequency(RPWM_PIN, 20000);
  analogWriteFrequency(LPWM_PIN, 20000);

  // 🔥 Smooth ramp-up
  for (int i = 0; i <= motorSpeed; i += 5) {
    motorForward(i);
    delay(50);
  }

  Serial.println("Motor running at high speed");
}

void loop() {
}
