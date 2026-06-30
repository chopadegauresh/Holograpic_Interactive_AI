#include <Adafruit_NeoPixel.h>
#include <math.h>

#define NUM_LEDS 36

Adafruit_NeoPixel strip1(NUM_LEDS, 2, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip2(NUM_LEDS, 3, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip3(NUM_LEDS, 4, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip4(NUM_LEDS, 5, NEO_GRB + NEO_KHZ800);

#define DELAY_TIME 10   // faster = smoother

int offset = 0;

// 🌈 RGB color wheel (smooth rainbow)
uint32_t colorWheel(byte pos) {
  pos = 255 - pos;
  if (pos < 85) {
    return strip1.Color(255 - pos * 3, 0, pos * 3);
  }
  if (pos < 170) {
    pos -= 85;
    return strip1.Color(0, pos * 3, 255 - pos * 3);
  }
  pos -= 170;
  return strip1.Color(pos * 3, 255 - pos * 3, 0);
}

// 🌊 Wave with dim-to-dark effect
uint32_t waveColor(int pos) {

  // brightness wave (0 → 1)
  float wave = (sin(pos * 0.25) + 1.0) / 2.0;

  // make it VERY dim (soft glow)
  float brightness = wave * 0.2;   // 🔥 0.2 = very dim

  byte colorIndex = pos % 255;
  uint32_t baseColor = colorWheel(colorIndex);

  byte r = ((baseColor >> 16) & 0xFF) * brightness;
  byte g = ((baseColor >> 8) & 0xFF) * brightness;
  byte b = (baseColor & 0xFF) * brightness;

  return strip1.Color(r, g, b);
}

// ===== Pattern with phase shift =====
void updateStrip(Adafruit_NeoPixel &s, int phase) {

  for (int i = 0; i < NUM_LEDS; i++) {

    int pos = i + offset + phase;

    s.setPixelColor(i, waveColor(pos));
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

  updateStrip(strip1, 0);
  updateStrip(strip2, 12);
  updateStrip(strip3, 24);
  updateStrip(strip4, 36);

  offset++;

  delay(DELAY_TIME);
}
