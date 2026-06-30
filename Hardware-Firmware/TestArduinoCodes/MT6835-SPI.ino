#include <Arduino.h>
#include <SimpleFOC.h>
#include <SPI.h>
#include <MagneticSensorMT6835.h>

// ===== SPI Pins (ESP32 VSPI) =====
#define MT6835_CS   5
#define MT6835_MISO 19
#define MT6835_MOSI 23
#define MT6835_SCK  18

// ===== SPI Settings =====
SPISettings mt6835SPISettings(1000000, MSBFIRST, SPI_MODE3);

// ===== Sensor Object =====
MagneticSensorMT6835 sensor = MagneticSensorMT6835(MT6835_CS, mt6835SPISettings);

// ===== Time tracking =====
unsigned long ts = 0;

void setup() {
  Serial.begin(115200);

  SPI.begin(MT6835_SCK, MT6835_MISO, MT6835_MOSI, MT6835_CS);

  sensor.init();

  Serial.println("✅ MT6835 + SimpleFOC Initialized");
}

void loop() {
  sensor.update();

  if (millis() - ts > 200) {
    ts = millis();

    // ===== ANGLE (-PI to +PI) =====
    float angle = sensor.getAngle();

    // ===== CONVERT TO 0 → 2PI =====
    if (angle < 0) angle += 2 * PI;

    // ===== CREATE RAW VALUE MANUALLY =====
    long raw = (long)((angle / (2 * PI)) * 2097152.0);

    // ===== VELOCITY =====
    float velocity = sensor.getVelocity();

    // ===== PRINT =====
    Serial.print("Raw(21bit): ");
    Serial.print(raw);

    Serial.print(" | Angle(rad 0-2PI): ");
    Serial.print(angle, 6);

    Serial.print(" | Velocity(rad/s): ");
    Serial.println(velocity, 6);
  }
}
