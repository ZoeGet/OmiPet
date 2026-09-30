#include <Arduino.h>
#include "NV3007_Display.h"
#include "led_strip.h"
#include "omi_pet_ui.h"

void setup() {
  OmiPetLed::strip.begin(16);
  OmiPetLed::strip.fill(255, 255, 255);

  OmiPetDisplay::lcd.begin(8000000UL);
  OmiPetDisplay::lcd.setBacklight(true);
  OmiPetUi::begin();
}

void loop() {
  OmiPetUi::update();
}
