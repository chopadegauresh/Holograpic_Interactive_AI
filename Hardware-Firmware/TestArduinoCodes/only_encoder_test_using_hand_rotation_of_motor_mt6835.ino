#include <Arduino.h>
#include <SPI.h>
#include <SimpleFOC.h>
#include <MagneticSensorMT6835.h>

// =========================
// MT6835 Connections
// CS   -> Teensy Pin 10
// MOSI -> Teensy Pin 11
// MISO -> Teensy Pin 12
// SCK  -> Teensy Pin 13
// VCC  -> 3.3V
// GND  -> GND
// =========================

#define MT6835_CS 10

SPISettings mt6835SPISettings(1000000, MSBFIRST, SPI_MODE3);
MagneticSensorMT6835 sensor(MT6835_CS, mt6835SPISettings);

unsigned long printTimer = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  SPI.begin();
  sensor.init();

  Serial.println("====================================");
  Serial.println("MT6835 Hand Rotation Test");
  Serial.println("Rotate the shaft slowly by hand");
  Serial.println("====================================");
}

void loop() {
  sensor.update();

  // Absolute angle for one rotation only
  float angle = sensor.getMechanicalAngle();

  // Convert to 0 -> 2PI
  if (angle < 0) {
    angle += 2.0f * PI;
  }

  // Convert to degrees
  float degrees = angle * 180.0f / PI;

  // Convert to 21-bit raw value
  long rawValue = (long)((angle / (2.0f * PI)) * 2097152.0f);

  // Velocity
  float velocity = sensor.getVelocity();

  // Print every 100 ms
  if (millis() - printTimer >= 100) {
    printTimer = millis();

    Serial.print("Raw21: ");
    Serial.print(rawValue);

    Serial.print(" | Angle(rad): ");
    Serial.print(angle, 6);

    Serial.print(" | Angle(deg): ");
    Serial.print(degrees, 2);

    Serial.print(" | Velocity(rad/s): ");
    Serial.println(velocity, 6);
  }
}
