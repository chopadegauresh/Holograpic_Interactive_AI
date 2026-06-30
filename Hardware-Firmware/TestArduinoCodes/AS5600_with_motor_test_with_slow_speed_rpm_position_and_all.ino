#include "AS5600.h"
#include <Wire.h>

AS5600 as5600;

long lastPosition = 0;
unsigned long lastTime = 0;

// Motor speed
int motorSpeedPWM = 9;
int startPWM = 30;

void setup()
{
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(400000);

  as5600.begin();
  as5600.setDirection(AS5600_CLOCK_WISE);

  as5600.resetCumulativePosition();
  lastPosition = as5600.getCumulativePosition();
  lastTime = millis();

  // BTN7960 pins
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);

  digitalWrite(8, HIGH);
  digitalWrite(9, HIGH);

  Serial.println("Motor starting...");

  analogWrite(5, startPWM);
  analogWrite(6, 0);

  delay(500);

  analogWrite(5, motorSpeedPWM);

  Serial.println("Motor running...");
}

void loop()
{
  unsigned long currentTime = millis();

  long currentPos = as5600.getCumulativePosition();
  int rawAngle = as5600.rawAngle();

  float angle = (rawAngle * 360.0) / 4096.0;

  if (currentPos != lastPosition)
  {
    long deltaPos = currentPos - lastPosition;
    float dt = currentTime - lastTime;

    float rpm = ((float)deltaPos / 4096.0) * (60000.0 / dt);

    Serial.print("Angle: ");
    Serial.print(angle, 2);
    Serial.print(" deg");

    Serial.print(" | Position Count: ");
    Serial.print(currentPos);

    if(deltaPos > 0)
      Serial.print(" | Direction: CW");
    else
      Serial.print(" | Direction: CCW");

    Serial.print(" | RPM: ");
    Serial.print(abs(rpm), 2);

    Serial.println();

    lastPosition = currentPos;
    lastTime = currentTime;
  }
}
