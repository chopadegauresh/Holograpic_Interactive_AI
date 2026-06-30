// Pin Definitions
const int RPWM = 5; // PWM output for Forward
const int LPWM = 6; // PWM output for Reverse
const int R_EN = 8; // Right Enable
const int L_EN = 9; // Left Enable

void setup() {
  pinMode(RPWM, OUTPUT);
  pinMode(LPWM, OUTPUT);
  pinMode(R_EN, OUTPUT);
  pinMode(L_EN, OUTPUT);
  
  // Enable the driver
  digitalWrite(R_EN, HIGH);
  digitalWrite(L_EN, HIGH);
  
  Serial.begin(9600);
  Serial.println("Motor Test Starting...");
}

void loop() {
  // Ramping Up Forward
  Serial.println("Forward Ramping Up...");
  for (int speed = 0; speed <= 255; speed++) {
    analogWrite(RPWM, speed);
    analogWrite(LPWM, 0); // Ensure other side is OFF
    delay(20); 
  }

  delay(2000); // Run at full speed for 2 seconds

  // Ramping Down
  Serial.println("Ramping Down...");
  for (int speed = 255; speed >= 0; speed--) {
    analogWrite(RPWM, speed);
    delay(20);
  }

  delay(1000); // Wait 1 second before repeat
}
