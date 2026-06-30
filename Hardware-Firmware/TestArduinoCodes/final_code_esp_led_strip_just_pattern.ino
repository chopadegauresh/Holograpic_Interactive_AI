#include <Adafruit_NeoPixel.h>
#include <math.h>

#define NUM_LEDS 36

Adafruit_NeoPixel strip1(NUM_LEDS, 2, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip2(NUM_LEDS, 3, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip3(NUM_LEDS, 4, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip4(NUM_LEDS, 5, NEO_GRB + NEO_KHZ800);

#define DELAY_TIME 10

int offset = 0;
int offBlade = 0;

// 🌈 Color wheel
uint32_t colorWheel(byte pos) {
  pos = 255 - pos;
  if (pos < 85) return strip1.Color(255 - pos * 3, 0, pos * 3);
  if (pos < 170) {
    pos -= 85;
    return strip1.Color(0, pos * 3, 255 - pos * 3);
  }
  pos -= 170;
  return strip1.Color(pos * 3, 255 - pos * 3, 0);
}

// 🌊 Dim wave
uint32_t waveColor(int pos) {
  float wave = (sin(pos * 0.25) + 1.0) / 2.0;
  float brightness = wave * 0.2;

  byte idx = pos % 255;
  uint32_t base = colorWheel(idx);

  byte r = ((base >> 16) & 0xFF) * brightness;
  byte g = ((base >> 8)  & 0xFF) * brightness;
  byte b = ( base        & 0xFF) * brightness;

  return strip1.Color(r, g, b);
}

// ===== PATTERN 1: NORMAL WAVE =====
void patternWave(Adafruit_NeoPixel &s, int phase) {
  for (int i = 0; i < NUM_LEDS; i++) {
    int pos = i + offset + phase;
    s.setPixelColor(i, waveColor(pos));
  }
}

// ===== PATTERN 2: INWARD =====
void patternInward(Adafruit_NeoPixel &s, int phase) {
  for (int i = 0; i < NUM_LEDS/2; i++) {
    int pos = i + offset + phase;

    s.setPixelColor(i, waveColor(pos));
    s.setPixelColor(NUM_LEDS - 1 - i, waveColor(pos));
  }
}

// ===== PATTERN 3: OUTWARD FROM CENTER =====
void patternOutward(Adafruit_NeoPixel &s, int phase) {
  int center = NUM_LEDS / 2;

  for (int i = 0; i < center; i++) {
    int pos = i + offset + phase;

    s.setPixelColor(center + i, waveColor(pos));
    s.setPixelColor(center - i - 1, waveColor(pos));
  }
}

// ===== DRAW FUNCTION =====
void drawBlade(Adafruit_NeoPixel &s, int phase, int type, bool off) {

  if (off) {
    s.clear();   // OFF blade
  } else {

    s.clear();

    if (type == 0) patternWave(s, phase);
    if (type == 1) patternInward(s, phase);
    if (type == 2) patternOutward(s, phase);
  }

  s.show();
}

void setup() {
  strip1.begin();
  strip2.begin();
  strip3.begin();
  strip4.begin();

  strip1.show();
  strip2.show();
  strip3.show();
  strip4.show();
}

void loop() {

  // assign behaviors
  drawBlade(strip1, 0, 0, offBlade == 0);  // wave
  drawBlade(strip2, 12,1, offBlade == 1);  // inward
  drawBlade(strip3, 24,2, offBlade == 2);  // outward
  drawBlade(strip4, 36,0, offBlade == 3);  // wave but can be OFF

  offset++;

  // rotate OFF blade clockwise
  static int counter = 0;
  counter++;

  if (counter > 8) {
    counter = 0;
    offBlade++;
    if (offBlade > 3) offBlade = 0;
  }

  delay(DELAY_TIME);
}
