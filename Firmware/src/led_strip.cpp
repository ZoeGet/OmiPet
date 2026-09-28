#include "led_strip.h"

namespace OmiPetLed {

Strip::Strip()
    : pixels_(kLedCount, kDataPin, NEO_GRB + NEO_KHZ800) {}

void Strip::begin(uint8_t brightness) {
  pixels_.begin();
  setBrightness(brightness);
  pixels_.clear();
  pixels_.show();
}

void Strip::clear(bool update) {
  pixels_.clear();
  if (update) pixels_.show();
}

void Strip::show() { pixels_.show(); }

void Strip::setBrightness(uint8_t brightness) {
  brightness_ = brightness;
  pixels_.setBrightness(brightness_);
}

uint8_t Strip::brightness() const { return brightness_; }

void Strip::setPixel(uint16_t index, uint8_t red, uint8_t green,
                     uint8_t blue) {
  if (index < kLedCount) pixels_.setPixelColor(index, pixels_.Color(red, green, blue));
}

void Strip::setPixel(uint16_t index, uint32_t color_value) {
  if (index < kLedCount) pixels_.setPixelColor(index, color_value);
}

void Strip::fill(uint8_t red, uint8_t green, uint8_t blue, bool update) {
  fill(color(red, green, blue), update);
}

void Strip::fill(uint32_t color_value, bool update) {
  pixels_.fill(color_value, 0, kLedCount);
  if (update) pixels_.show();
}

void Strip::setPixelHSV(uint16_t index, uint8_t hue, uint8_t saturation,
                        uint8_t value) {
  if (index < kLedCount) {
    pixels_.setPixelColor(index, pixels_.ColorHSV(static_cast<uint16_t>(hue) * 256U,
                                                  saturation, value));
  }
}

uint32_t Strip::color(uint8_t red, uint8_t green, uint8_t blue) const {
  return pixels_.Color(red, green, blue);
}

uint32_t Strip::colorWheel(uint8_t position) const {
  position = 255 - position;
  if (position < 85) return color(255 - position * 3, 0, position * 3);
  if (position < 170) {
    position -= 85;
    return color(0, position * 3, 255 - position * 3);
  }
  position -= 170;
  return color(position * 3, 255 - position * 3, 0);
}

Adafruit_NeoPixel& Strip::pixels() { return pixels_; }
const Adafruit_NeoPixel& Strip::pixels() const { return pixels_; }

Strip strip;

}  // namespace OmiPetLed
