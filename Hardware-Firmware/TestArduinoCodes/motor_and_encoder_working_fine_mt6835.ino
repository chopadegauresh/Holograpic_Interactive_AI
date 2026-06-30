#include <Arduino.h>
#include <SPI.h>
#include <MagneticSensorMT6835.h>

// =========================
// MT6835 Encoder
// =========================
#define MT6835_CS 10

// Teensy 4.1 SPI Pins
// MOSI = 11
// MISO = 12
// SCK  = 13

SPISettings mt6835SPISettings(1000000, MSBFIRST, SPI_MODE3);
MagneticSensorMT6835 sensor(MT6835_CS, mt6835SPISettings);

// =========================
// BTN7960B Motor Driver Pins
// =========================
#define RPWM_PIN 6
#define LPWM_PIN 7
#define REN_PIN  8
#define LEN_PIN  9

// =========================
// Variables
// =========================
unsigned long printTimer = 0;

// =========================
// Motor Functions
// =========================
void motorStop() {
  analogWrite(RPWM_PIN, 0);
  analogWrite(LPWM_PIN, 0);
}

void motorForwardFull() {
  analogWrite(RPWM_PIN, 255);   // full forward speed
  analogWrite(LPWM_PIN, 0);
}

// =========================
// Setup
// =========================
void setup() {
  Serial.begin(115200);
  delay(1000);

  // Motor driver setup
  pinMode(RPWM_PIN, OUTPUT);
  pinMode(LPWM_PIN, OUTPUT);
  pinMode(REN_PIN, OUTPUT);
  pinMode(LEN_PIN, OUTPUT);

  digitalWrite(REN_PIN, HIGH);
  digitalWrite(LEN_PIN, HIGH);

  analogWriteFrequency(RPWM_PIN, 20000);
  analogWriteFrequency(LPWM_PIN, 20000);

  motorStop();

  // SPI + Encoder setup
  SPI.begin();
  sensor.init();

  delay(500);

  // Start motor at full forward speed
  motorForwardFull();

  Serial.println("====================================");
  Serial.println("Teensy 4.1 + MT6835 + BTN7960B");
  Serial.println("====================================");
}

// =========================
// Loop
// =========================
void loop() {

  // Update encoder reading
  sensor.update();

  // Read angle in radians
  float angle = sensor.getMechanicalAngle();

  // Convert negative angle to 0 -> 2PI
  if (angle < 0) {
    angle += 2.0f * PI;
  }

  // Convert to degrees
  float angleDeg = angle * 180.0f / PI;

  // Convert to 21-bit encoder value
  uint32_t raw21 = (uint32_t)((angle / (2.0f * PI)) * 2097151.0f);

  // Velocity
  float velocity = sensor.getVelocity();

  // Print every 100 ms
  if (millis() - printTimer >= 100) {
    printTimer = millis();

    Serial.print("Raw21: ");
    Serial.print(raw21);

    Serial.print(" | Angle(deg): ");
    Serial.print(angleDeg, 2);

    Serial.print(" | Velocity(rad/s): ");
    Serial.println(velocity, 4);
  }
}
