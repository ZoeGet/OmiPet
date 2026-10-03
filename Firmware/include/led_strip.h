#pragma once

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

//  WS2812B 灯带驱动接口 / WS2812B LED strip driver interface
namespace OmiPetLed {

//  最新原理图映射：LED 网络连接到 ESP32-S3 GPIO47 / Latest schematic mapping: the LED net connects to ESP32-S3 GPIO47
constexpr uint8_t kDataPin = 47;  //  WS2812B 数据引脚 / WS2812B data pin
constexpr uint16_t kLedCount = 13;  //  原理图中的 LED2 至 LED14 / LED2 through LED14 in the schematic

class Strip {
 public:
  //  创建默认灯带对象 / Create a default strip object
  Strip();

  //  初始化灯带和亮度 / Initialize the strip and set brightness
  void begin(uint8_t brightness = 64);  //  初始全局亮度，范围 0–255 / Initial global brightness from 0 to 255
  //  清空灯带并按需立即刷新 / Clear the strip and optionally refresh immediately
  void clear(bool update = true);
  //  将待发送颜色数据写入灯带 / Send the pending color data to the strip
  void show();
  //  设置全局亮度 / Set the global brightness
  void setBrightness(uint8_t brightness);
  //  获取当前全局亮度 / Get the current global brightness
  uint8_t brightness() const;

  //  按 RGB 分量设置单颗灯珠 / Set one LED by RGB components
  void setPixel(uint16_t index, uint8_t red, uint8_t green, uint8_t blue);
  //  使用打包颜色设置单颗灯珠 / Set one LED using a packed color
  void setPixel(uint16_t index, uint32_t color);
  //  用 RGB 颜色填充全部灯珠 / Fill all LEDs with an RGB color
  void fill(uint8_t red, uint8_t green, uint8_t blue, bool update = true);
  //  用打包颜色填充全部灯珠 / Fill all LEDs with a packed color
  void fill(uint32_t color, bool update = true);
  //  按 HSV 分量设置单颗灯珠 / Set one LED by HSV components
  void setPixelHSV(uint16_t index, uint8_t hue, uint8_t saturation,
                   uint8_t value);

  //  将 RGB 分量转换为库使用的颜色值 / Convert RGB components to a library color
  uint32_t color(uint8_t red, uint8_t green, uint8_t blue) const;
  //  根据色轮位置生成彩虹颜色 / Generate a rainbow color from a wheel position
  uint32_t colorWheel(uint8_t position) const;

  //  获取可修改的底层 NeoPixel 对象 / Get the mutable underlying NeoPixel object
  Adafruit_NeoPixel& pixels();
  //  获取只读的底层 NeoPixel 对象 / Get the read-only underlying NeoPixel object
  const Adafruit_NeoPixel& pixels() const;

 private:
  Adafruit_NeoPixel pixels_;
  uint8_t brightness_ = 64;  //  当前灯带全局亮度 / Current global strip brightness
};

extern Strip strip;

}  //  OmiPetLed 命名空间 / OmiPetLed namespace
