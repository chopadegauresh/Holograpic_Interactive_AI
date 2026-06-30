const int RPWM = 5; 
const int LPWM = 6; 
const int R_EN = 8; 
const int L_EN = 9; 

void setup() {
  pinMode(RPWM, OUTPUT);
  pinMode(LPWM, OUTPUT);
  pinMode(R_EN, OUTPUT);
  pinMode(L_EN, OUTPUT);

  digitalWrite(R_EN, HIGH);
  digitalWrite(L_EN, HIGH);
  Serial.begin(9600);
}

void moveMotor(int speed) {
  if (speed > 0) {
    // Forward
    analogWrite(RPWM, speed);
    analogWrite(LPWM, 0);
  } else if (speed < 0) {
    // Reverse (use absolute value for speed)
    analogWrite(RPWM, 0);
    analogWrite(LPWM, abs(speed));
  } else {
    // Stop
    analogWrite(RPWM, 0);
    analogWrite(LPWM, 0);
  }
}

void loop() {
  Serial.println("Full Speed Forward!");
  moveMotor(255); 
  delay(3000);

  Serial.println("Braking...");
  moveMotor(0);
  delay(1000);

  Serial.println("Full Speed Reverse!");
  moveMotor(-255); // Notice the negative sign
  delay(3000);

  Serial.println("Braking...");
  moveMotor(0);
  delay(1000);
}
