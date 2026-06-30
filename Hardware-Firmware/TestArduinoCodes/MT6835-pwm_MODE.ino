#define PWM_PIN 34

void setup() {
  Serial.begin(115200);
  pinMode(PWM_PIN, INPUT);
}

void loop() {
  uint32_t highTime = pulseIn(PWM_PIN, HIGH);
  uint32_t lowTime  = pulseIn(PWM_PIN, LOW);

  uint32_t period = highTime + lowTime;

  if(period > 0){
    float duty = (float)highTime / period;
    float angle = duty * 360.0;

    Serial.print("Angle: ");
    Serial.println(angle);
  }
}
