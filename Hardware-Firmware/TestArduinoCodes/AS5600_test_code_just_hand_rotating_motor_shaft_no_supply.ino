#include "AS5600.h"
#include <Wire.h>

AS5600 as5600;

long lastPosition = 0;

void setup() 
{
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(400000);

  as5600.begin();
  as5600.setDirection(AS5600_CLOCK_WISE);

  // Reset cumulative counter
  as5600.resetCumulativePosition();

  lastPosition = as5600.getCumulativePosition();

  Serial.println("Rotate the motor shaft by hand...");
}

void loop() 
{
  long currentPos = as5600.getCumulativePosition();
  int rawAngle = as5600.rawAngle();   // 0 - 4095

  float angle = (rawAngle * 360.0) / 4096.0;

  if (currentPos != lastPosition)
  {
    Serial.print("Angle: ");
    Serial.print(angle, 2);
    Serial.print(" deg");

    Serial.print(" | Position Count: ");
    Serial.print(currentPos);

    if(currentPos > lastPosition)
      Serial.print(" | Direction: CW");
    else
      Serial.print(" | Direction: CCW");

    Serial.println();

    lastPosition = currentPos;
  }
}
