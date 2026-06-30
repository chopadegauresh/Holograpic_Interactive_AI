#include <Wire.h>

#define MPU 0x68
#define A_R 16384.0   // Accelerometer sensitivity (±2g)
#define G_R 131.0     // Gyroscope sensitivity (±250°/s)

int16_t AcX, AcY, AcZ;
int16_t GyX, GyY, GyZ;

float Acc[2];
float Gy[3];
float Angle[3];

unsigned long tiempo_prev;
float dt;

void setup() {
  Serial.begin(115200);

  // ESP32 I2C pins
  Wire.begin(21, 22);  // SDA, SCL

  // Wake up MPU6050
  Wire.beginTransmission(MPU);
  Wire.write(0x6B);   // Power management register
  Wire.write(0);      // Wake up MPU6050
  Wire.endTransmission(true);

  tiempo_prev = millis();
  Serial.println("MPU6050 ready (ESP32)");
}

void loop() {

  // --------- Read Accelerometer ----------
  Wire.beginTransmission(MPU);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU, 6, true);

  AcX = Wire.read() << 8 | Wire.read();
  AcY = Wire.read() << 8 | Wire.read();
  AcZ = Wire.read() << 8 | Wire.read();

  Acc[0] = atan((AcY / A_R) /
           sqrt(pow((AcX / A_R), 2) + pow((AcZ / A_R), 2))) * RAD_TO_DEG;

  Acc[1] = atan(-1 * (AcX / A_R) /
           sqrt(pow((AcY / A_R), 2) + pow((AcZ / A_R), 2))) * RAD_TO_DEG;

  // --------- Read Gyroscope ----------
  Wire.beginTransmission(MPU);
  Wire.write(0x43);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU, 6, true);

  GyX = Wire.read() << 8 | Wire.read();
  GyY = Wire.read() << 8 | Wire.read();
  GyZ = Wire.read() << 8 | Wire.read();

  Gy[0] = GyX / G_R;
  Gy[1] = GyY / G_R;
  Gy[2] = GyZ / G_R;

  // --------- Time step ----------
  dt = (millis() - tiempo_prev) / 1000.0;
  tiempo_prev = millis();

  // --------- Complementary Filter ----------
  Angle[0] = 0.98 * (Angle[0] + Gy[0] * dt) + 0.02 * Acc[0]; // Roll
  Angle[1] = 0.98 * (Angle[1] + Gy[1] * dt) + 0.02 * Acc[1]; // Pitch

  // --------- Yaw (gyro only) ----------
  Angle[2] += Gy[2] * dt;

  // --------- Serial Output ----------
  Serial.print("Roll: ");
  Serial.print(Angle[0]);
  Serial.print("  Pitch: ");
  Serial.print(Angle[1]);
  Serial.print("  Yaw: ");
  Serial.println(Angle[2]);

  delay(10);
}
