// Pin Definitions
const int RPWM = 5; // Forward PWM
const int LPWM = 6; // Reverse PWM
const int R_EN = 8; // Right Enable
const int L_EN = 9; // Left Enable

void setup() {
  // Initialize pins
  pinMode(RPWM, OUTPUT);
  pinMode(LPWM, OUTPUT);
  pinMode(R_EN, OUTPUT);
  pinMode(L_EN, OUTPUT);

  // 1. Enable the driver (Wake it up)
  digitalWrite(R_EN, HIGH);
  digitalWrite(L_EN, HIGH);

  // 2. Set Max Speed (255 is 100% Duty Cycle)
  // To spin at 3500 RPM, we send full power to RPWM and 0 to LPWM
  analogWrite(RPWM, 255); 
  analogWrite(LPWM, 0);

  Serial.begin(9600);
  Serial.println("Motor initialized at MAX speed (3500 RPM).");
  Serial.println("Running continuously...");
}

void loop() {
  // We leave the loop empty because the setup() 
  // has already set the hardware pins to HIGH.
}
