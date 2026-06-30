#include <Arduino.h>
#include <SPI.h>
#include <SimpleFOC.h>
#include <MagneticSensorMT6835.h>

// --- Pins ---
#define MT6835_CS 10
#define RPWM_PIN  6
#define LPWM_PIN  7
#define REN_PIN   8
#define LEN_PIN   9

// --- Settings ---
SPISettings mt6835SPISettings(1000000, MSBFIRST, SPI_MODE3);
MagneticSensorMT6835 sensor(MT6835_CS, mt6835SPISettings);

unsigned long printTimer = 0;

void setup() {
  Serial.begin(115200);
  
  // Initialize Pins
  pinMode(RPWM_PIN, OUTPUT);
  pinMode(LPWM_PIN, OUTPUT);
  pinMode(REN_PIN, OUTPUT);
  pinMode(LEN_PIN, OUTPUT);

  // Set Teensy PWM frequency (BTS7960 works best between 10kHz - 20kHz)
  analogWriteFrequency(RPWM_PIN, 15000);
  analogWriteFrequency(LPWM_PIN, 15000);

  // Enable Driver
  digitalWrite(REN_PIN, HIGH);
  digitalWrite(LEN_PIN, HIGH);

  // Initialize Encoder
  SPI.begin();
  sensor.init();

  Serial.println("SYSTEM START: FORWARD FULL SPEED MODE");
}

void loop() {
  // 1. UPDATE SENSOR
  sensor.update();

  // 2. FORWARD FULL SPEED
  // 255 = 100% Duty Cycle (Full 12V to motor)
  analogWrite(RPWM_PIN, 255); 
  analogWrite(LPWM_PIN, 0);

  // 3. CALCULATE TELEMETRY
  float angle = sensor.getMechanicalAngle();
  if (angle < 0) angle += 2.0f * PI;
  
  float velocity = sensor.getVelocity();
  float rpm = (velocity * 60.0f) / (2.0f * PI);

  // 4. PRINT DATA (Every 100ms)
  if (millis() - printTimer > 100) {
    printTimer = millis();
    
    Serial.print("V_RAD: ");
    Serial.print(velocity, 2);
    Serial.print(" | RPM: ");
    Serial.print(rpm, 0);
    Serial.print(" | DEG: ");
    Serial.println(angle * 180.0 / PI, 2);
  }
}
