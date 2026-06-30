#include <Adafruit_NeoPixel.h>

#define NUM_LEDS 36

Adafruit_NeoPixel strip1(NUM_LEDS, 2, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip2(NUM_LEDS, 3, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip3(NUM_LEDS, 4, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip4(NUM_LEDS, 5, NEO_GRB + NEO_KHZ800);

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

void fillStrip(Adafruit_NeoPixel &s, uint32_t color) {
  for(int i=0;i<NUM_LEDS;i++) s.setPixelColor(i, color);
  s.show();
}

void loop() {
  fillStrip(strip1, strip1.Color(255,0,0));
  fillStrip(strip2, strip2.Color(0,0,0));
  fillStrip(strip3, strip3.Color(0,0,0));
  fillStrip(strip4, strip4.Color(0,0,0));
  delay(1000);

  fillStrip(strip1, strip1.Color(0,0,0));
  fillStrip(strip2, strip2.Color(0,255,0));
  delay(1000);

  fillStrip(strip2, strip2.Color(0,0,0));
  fillStrip(strip3, strip3.Color(0,0,255));
  delay(1000);

  fillStrip(strip3, strip3.Color(0,0,0));
  fillStrip(strip4, strip4.Color(255,255,255));
  delay(1000);

  fillStrip(strip4, strip4.Color(0,0,0));
}
