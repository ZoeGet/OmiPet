#pragma once

#include <Arduino.h>
#include <SPI.h>

namespace OmiPetDisplay {

// 屏幕逻辑分辨率 / Logical display resolution
constexpr uint16_t kPanelWidth = 142;
constexpr uint16_t kPanelHeight = 428;

// LCD SPI 和控制引脚 / LCD SPI and control pins
constexpr uint8_t kMosiPin = 11;
constexpr uint8_t kSckPin = 12;
constexpr uint8_t kDcPin = 9;
constexpr uint8_t kCsPin = 10;
constexpr uint8_t kResetPin = 14;
constexpr uint8_t kBacklightPin = 21;

// 背光有效电平 / Backlight active level
constexpr bool kBacklightActiveHigh = true;

// NV3007 四线 SPI 显示驱动 / NV3007 four-wire SPI display driver
class Display {
 public:
  Display();

  // 初始化 GPIO、SPI、复位时序和面板寄存器 / Initialize GPIO, SPI, reset timing, and panel registers
  bool begin(uint32_t frequency = 8000000UL);

  // 设置显示方向 / Set display rotation
  void setRotation(uint8_t rotation);

  // 控制背光开关 / Control the backlight
  void setBacklight(bool enabled);

  // 设置包含首尾坐标的显示窗口 / Set an inclusive display address window
  void setAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1,
                        uint16_t y1);

  // 填充整屏颜色 / Fill the entire screen with one color
  void fillScreen(uint16_t color);

  // 填充矩形区域 / Fill a rectangular area
  void fillRect(int16_t x, int16_t y, int16_t width, int16_t height,
                uint16_t color);

  // 绘制 RGB565 位图 / Draw an RGB565 bitmap
  void drawBitmap(int16_t x, int16_t y, uint16_t width, uint16_t height,
                  const uint16_t* pixels);

  // 绘制单个像素 / Draw one pixel
  void drawPixel(int16_t x, int16_t y, uint16_t color);

  // 绘制水平线 / Draw a horizontal line
  void drawFastHLine(int16_t x, int16_t y, int16_t width, uint16_t color);

  // 绘制垂直线 / Draw a vertical line
  void drawFastVLine(int16_t x, int16_t y, int16_t height, uint16_t color);

  // 绘制矩形边框 / Draw a rectangle outline
  void drawRect(int16_t x, int16_t y, int16_t width, int16_t height,
                uint16_t color);

  // 获取当前逻辑宽度 / Get the current logical width
  uint16_t width() const;

  // 获取当前逻辑高度 / Get the current logical height
  uint16_t height() const;

  // 查询驱动是否已初始化 / Check whether the driver is initialized
  bool initialized() const;

  // 将 8 位 RGB 转换为 RGB565 / Convert 8-bit RGB to RGB565
  static constexpr uint16_t color565(uint8_t red, uint8_t green,
                                     uint8_t blue) {
    return static_cast<uint16_t>(((red & 0xF8U) << 8) |
                                 ((green & 0xFCU) << 3) | (blue >> 3));
  }

  struct InitCommand {
    uint8_t command;
    uint8_t data[5];
    uint8_t dataLength;
    uint16_t delayMs;
  };

 private:
  void writeCommand(uint8_t command);
  void writeCommandData(uint8_t command, const uint8_t* data,
                        size_t length);
  void writeColor(uint16_t color, uint32_t count);
  void writeInitSequence();
  void setAddressWindowUnchecked(uint16_t x0, uint16_t y0, uint16_t x1,
                                 uint16_t y1);

  SPIClass& spi_;
  SPISettings settings_;
  uint32_t frequency_ = 8000000UL;
  uint8_t rotation_ = 0;
  uint8_t madctl_ = 0x00;
  uint16_t width_ = kPanelWidth;
  uint16_t height_ = kPanelHeight;
  uint16_t xOffset_ = 12;
  uint16_t yOffset_ = 0;
  bool initialized_ = false;
};

extern Display lcd;

}  // OmiPetDisplay 命名空间 / OmiPetDisplay namespace
