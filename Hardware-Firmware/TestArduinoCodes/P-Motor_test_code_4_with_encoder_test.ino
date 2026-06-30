#include "AS5600.h"
#include <Wire.h>

AS5600 as5600;

unsigned long lastTime = 0;
long lastPosition = 0;

// Adjustment: Change this to find your motor's "sweet spot" for 50 RPM
int motorSpeedPWM = 40; 

void setup() {
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(400000); 

  as5600.begin();
  as5600.setDirection(AS5600_CLOCK_WISE);
  as5600.setHysteresis(3); 
  as5600.setSlowFilter(0); 
  
  as5600.resetCumulativePosition(); 
  lastPosition = as5600.getCumulativePosition();

  pinMode(5, OUTPUT); pinMode(6, OUTPUT);
  pinMode(8, OUTPUT); pinMode(9, OUTPUT);
  digitalWrite(8, HIGH); digitalWrite(9, HIGH);
  
  // Start with a low PWM value
  analogWrite(5, motorSpeedPWM); 
  analogWrite(6, 0);

  Serial.println("Monitoring slow speed (Target 50 RPM)...");
}

void loop() {
  unsigned long currentTime = millis();

  // For slow speeds, we check every 200ms to allow the encoder to count enough steps
  if (currentTime - lastTime >= 200) {
    long currentPos = as5600.getCumulativePosition();
    int agc = as5600.readAGC(); 
    
    long deltaPos = currentPos - lastPosition; 
    float dt = (currentTime - lastTime);
    
    // RPM Calculation
    float rpm = ((float)deltaPos / 4096.0) * (60000.0 / dt);

    Serial.print("RPM: ");
    Serial.print(abs(rpm), 1); // Show one decimal place for accuracy at low speed
    Serial.print(" | PWM Value: ");
    Serial.print(motorSpeedPWM);
    Serial.print(" | AGC: ");
    Serial.println(agc);

    lastPosition = currentPos;
    lastTime = currentTime;
  }
}
