#include <Arduino.h>
#include "led_strip.h"

void setup() {
  OmiPetLed::strip.begin(64);
  OmiPetLed::strip.clear();
}

void loop() {
  // Intentionally idle until the application effect policy is defined.
}
