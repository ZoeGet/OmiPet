#pragma once

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

namespace OmiPetLed {

//  最新原理图映射：LED 网络连接到 ESP32-S3 GPIO47 / Latest schematic mapping: the LED net connects to ESP32-S3 GPIO47
constexpr uint8_t kDataPin = 47;
constexpr uint16_t kLedCount = 13; //  原理图中的 LED2 至 LED14 / LED2 through LED14 in the schematic

class Strip {
 public:
  Strip();

  void begin(uint8_t brightness = 64);
  void clear(bool update = true);
  void show();
  void setBrightness(uint8_t brightness);
  uint8_t brightness() const;

  void setPixel(uint16_t index, uint8_t red, uint8_t green, uint8_t blue);
  void setPixel(uint16_t index, uint32_t color);
  void fill(uint8_t red, uint8_t green, uint8_t blue, bool update = true);
  void fill(uint32_t color, bool update = true);
  void setPixelHSV(uint16_t index, uint8_t hue, uint8_t saturation,
                   uint8_t value);

  uint32_t color(uint8_t red, uint8_t green, uint8_t blue) const;
  uint32_t colorWheel(uint8_t position) const;

  Adafruit_NeoPixel& pixels();
  const Adafruit_NeoPixel& pixels() const;

 private:
  Adafruit_NeoPixel pixels_;
  uint8_t brightness_ = 64;
};

extern Strip strip;

}  //  OmiPetLed 命名空间 / OmiPetLed namespace
