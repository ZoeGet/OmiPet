#include "NV3007_Display.h"

namespace OmiPetDisplay {
namespace {

//  NV3007 常用命令 / Common NV3007 commands
constexpr uint8_t kCommandMemoryAccessControl = 0x36;
constexpr uint8_t kCommandColumnAddressSet = 0x2A;
constexpr uint8_t kCommandPageAddressSet = 0x2B;
constexpr uint8_t kCommandMemoryWrite = 0x2C;

//  各旋转方向对应的 MADCTL 值 / MADCTL values for each rotation
constexpr uint8_t kMadctlValues[] = {0x00, 0xC0, 0x60, 0xA0};

//  厂家提供的 NV3007 初始化序列 / NV3007 initialization sequence from the panel vendor
constexpr Display::InitCommand kInitSequence[] = {
    {0xFF, {0xA5, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0x9A, {0x08, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0x9B, {0x08, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0x9C, {0xB0, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0x9D, {0x16, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0x9E, {0xC4, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0x8F, {0x55, 0x04, 0x0, 0x0, 0x0}, 2, 0},
    {0x84, {0x90, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0x83, {0x7B, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0x85, {0x33, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0x60, {0x00, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0x70, {0x00, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0x61, {0x02, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0x71, {0x02, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0x62, {0x04, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0x72, {0x04, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0x6C, {0x29, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0x7C, {0x29, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0x6D, {0x31, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0x7D, {0x31, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0x6E, {0x0F, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0x7E, {0x0F, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0x66, {0x21, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0x76, {0x21, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0x68, {0x3A, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0x78, {0x3A, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0x63, {0x07, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0x73, {0x07, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0x64, {0x05, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0x74, {0x05, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0x65, {0x02, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0x75, {0x02, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0x67, {0x23, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0x77, {0x23, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0x69, {0x08, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0x79, {0x08, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0x6A, {0x13, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0x7A, {0x13, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0x6B, {0x13, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0x7B, {0x13, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0x6F, {0x00, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0x7F, {0x00, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0x50, {0x00, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0x52, {0xD6, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0x53, {0x08, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0x54, {0x08, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0x55, {0x1E, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0x56, {0x1C, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0xA0, {0x2B, 0x24, 0x00, 0x0, 0x0}, 3, 0},
    {0xA1, {0x87, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0xA2, {0x86, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0xA5, {0x00, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0xA6, {0x00, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0xA7, {0x00, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0xA8, {0x36, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0xA9, {0x7E, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0xAA, {0x7E, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0xB9, {0x85, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0xBA, {0x84, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0xBB, {0x83, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0xBC, {0x82, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0xBD, {0x81, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0xBE, {0x80, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0xBF, {0x01, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0xC0, {0x02, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0xC1, {0x00, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0xC2, {0x00, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0xC3, {0x00, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0xC4, {0x33, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0xC5, {0x7E, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0xC6, {0x7E, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0xC8, {0x33, 0x33, 0x0, 0x0, 0x0}, 2, 0},
    {0xC9, {0x68, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0xCA, {0x69, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0xCB, {0x6A, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0xCC, {0x6B, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0xCD, {0x33, 0x33, 0x0, 0x0, 0x0}, 2, 0},
    {0xCE, {0x6C, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0xCF, {0x6D, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0xD0, {0x6E, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0xD1, {0x6F, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0xAB, {0x03, 0x67, 0x0, 0x0, 0x0}, 2, 0},
    {0xAC, {0x03, 0x6B, 0x0, 0x0, 0x0}, 2, 0},
    {0xAD, {0x03, 0x68, 0x0, 0x0, 0x0}, 2, 0},
    {0xAE, {0x03, 0x6C, 0x0, 0x0, 0x0}, 2, 0},
    {0xB3, {0x00, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0xB4, {0x00, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0xB5, {0x00, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0xB6, {0x32, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0xB7, {0x7E, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0xB8, {0x7E, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0xE0, {0x00, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0xE1, {0x03, 0x0F, 0x0, 0x0, 0x0}, 2, 0},
    {0xE2, {0x04, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0xE3, {0x01, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0xE4, {0x0E, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0xE5, {0x01, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0xE6, {0x19, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0xE7, {0x10, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0xE8, {0x10, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0xEA, {0x12, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0xEB, {0xD0, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0xEC, {0x04, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0xED, {0x07, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0xEE, {0x07, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0xEF, {0x09, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0xF0, {0xD0, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0xF1, {0x0E, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0xF9, {0x17, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0xF2, {0x2C, 0x1B, 0x0B, 0x20, 0x0}, 4, 0},
    {0xE9, {0x29, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0xEC, {0x04, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0x35, {0x00, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0x44, {0x00, 0x10, 0x0, 0x0, 0x0}, 2, 0},
    {0x46, {0x10, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0xFF, {0x00, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0x3A, {0x05, 0x0, 0x0, 0x0, 0x0}, 1, 0},
    {0x11, {0x0, 0x0, 0x0, 0x0, 0x0}, 0, 220},
    {0x29, {0x0, 0x0, 0x0, 0x0, 0x0}, 0, 200},
};

}  //  匿名命名空间 / Anonymous namespace

Display::Display()
    : spi_(SPI), settings_(SPISettings(8000000UL, MSBFIRST, SPI_MODE0)) {}

bool Display::begin(uint32_t frequency) {
  frequency_ = frequency;
  settings_ = SPISettings(frequency_, MSBFIRST, SPI_MODE0);

  //  配置控制引脚并设置安全默认电平 / Configure control pins and safe default levels
  pinMode(kCsPin, OUTPUT);
  pinMode(kDcPin, OUTPUT);
  pinMode(kResetPin, OUTPUT);
  pinMode(kBacklightPin, OUTPUT);
  digitalWrite(kCsPin, HIGH);
  digitalWrite(kDcPin, HIGH);
  digitalWrite(kResetPin, HIGH);
  setBacklight(false);

  spi_.begin(kSckPin, -1, kMosiPin, kCsPin);

  //  执行硬件复位 / Perform a hardware reset
  digitalWrite(kResetPin, LOW);
  delay(100);
  digitalWrite(kResetPin, HIGH);
  delay(100);

  //  写入面板寄存器并应用默认方向 / Write panel registers and apply the default rotation
  writeInitSequence();
  setRotation(0);
  initialized_ = true;
  return true;
}

void Display::setRotation(uint8_t rotation) {
  rotation_ = rotation & 0x03U;
  madctl_ = kMadctlValues[rotation_];

  if ((rotation_ & 0x01U) == 0) {
    width_ = kPanelWidth;
    height_ = kPanelHeight;
    xOffset_ = 12;
    yOffset_ = 0;
  } else {
    width_ = kPanelHeight;
    height_ = kPanelWidth;
    xOffset_ = 0;
    yOffset_ = 0;
  }

  const uint8_t data[] = {madctl_};
  writeCommandData(kCommandMemoryAccessControl, data, sizeof(data));
}

void Display::setBacklight(bool enabled) {
  const bool level = enabled ? kBacklightActiveHigh : !kBacklightActiveHigh;
  digitalWrite(kBacklightPin, level ? HIGH : LOW);
}

void Display::writeCommand(uint8_t command) {
  //  发送无参数命令 / Send a command without parameters
  spi_.beginTransaction(settings_);
  digitalWrite(kCsPin, LOW);
  digitalWrite(kDcPin, LOW);
  spi_.transfer(command);
  digitalWrite(kCsPin, HIGH);
  spi_.endTransaction();
}

void Display::writeCommandData(uint8_t command, const uint8_t* data,
                               size_t length) {
  //  发送命令及参数 / Send a command followed by parameters
  spi_.beginTransaction(settings_);
  digitalWrite(kCsPin, LOW);
  digitalWrite(kDcPin, LOW);
  spi_.transfer(command);
  if (length > 0) {
    digitalWrite(kDcPin, HIGH);
    for (size_t index = 0; index < length; ++index) {
      spi_.transfer(data[index]);
    }
  }
  digitalWrite(kCsPin, HIGH);
  spi_.endTransaction();
}

void Display::writeInitSequence() {
  //  按顺序执行厂家初始化表 / Execute the vendor initialization table in order
  for (const InitCommand& item : kInitSequence) {
    writeCommandData(item.command, item.data, item.dataLength);
    if (item.delayMs > 0) {
      delay(item.delayMs);
    }
  }
}

void Display::setAddressWindowUnchecked(uint16_t x0, uint16_t y0,
                                         uint16_t x1, uint16_t y1) {
  //  坐标已经过调用方检查 / Coordinates are validated by the caller
  const uint8_t columnData[] = {
      static_cast<uint8_t>((x0 + xOffset_) >> 8),
      static_cast<uint8_t>(x0 + xOffset_),
      static_cast<uint8_t>((x1 + xOffset_) >> 8),
      static_cast<uint8_t>(x1 + xOffset_),
  };
  const uint8_t rowData[] = {
      static_cast<uint8_t>((y0 + yOffset_) >> 8),
      static_cast<uint8_t>(y0 + yOffset_),
      static_cast<uint8_t>((y1 + yOffset_) >> 8),
      static_cast<uint8_t>(y1 + yOffset_),
  };
  writeCommandData(kCommandColumnAddressSet, columnData, sizeof(columnData));
  writeCommandData(kCommandPageAddressSet, rowData, sizeof(rowData));
  writeCommand(kCommandMemoryWrite);
}

void Display::setAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1,
                               uint16_t y1) {
  if (!initialized_ || x0 > x1 || y0 > y1 || x1 >= width_ || y1 >= height_) {
    return;
  }
  setAddressWindowUnchecked(x0, y0, x1, y1);
}

void Display::writeColor(uint16_t color, uint32_t count) {
  const uint8_t high = static_cast<uint8_t>(color >> 8);
  const uint8_t low = static_cast<uint8_t>(color);
  spi_.beginTransaction(settings_);
  digitalWrite(kCsPin, LOW);
  digitalWrite(kDcPin, HIGH);
  while (count-- > 0) {
    spi_.transfer(high);
    spi_.transfer(low);
  }
  digitalWrite(kCsPin, HIGH);
  spi_.endTransaction();
}

void Display::fillScreen(uint16_t color) {
  if (!initialized_) {
    return;
  }
  setAddressWindowUnchecked(0, 0, width_ - 1, height_ - 1);
  writeColor(color, static_cast<uint32_t>(width_) * height_);
}

void Display::fillRect(int16_t x, int16_t y, int16_t width, int16_t height,
                       uint16_t color) {
  if (!initialized_ || width <= 0 || height <= 0) {
    return;
  }
  if (x < 0) {
    width += x;
    x = 0;
  }
  if (y < 0) {
    height += y;
    y = 0;
  }
  if (x + width > this->width_) {
    width = this->width_ - x;
  }
  if (y + height > this->height_) {
    height = this->height_ - y;
  }
  if (width <= 0 || height <= 0) {
    return;
  }
  setAddressWindowUnchecked(static_cast<uint16_t>(x), static_cast<uint16_t>(y),
                            static_cast<uint16_t>(x + width - 1),
                            static_cast<uint16_t>(y + height - 1));
  writeColor(color, static_cast<uint32_t>(width) * height);
}

void Display::drawBitmap(int16_t x, int16_t y, uint16_t width,
                         uint16_t height, const uint16_t* pixels) {
  if (!initialized_ || width == 0 || height == 0 || pixels == nullptr) {
    return;
  }

  const uint16_t sourceWidth = width;
  int16_t sourceX = 0;
  int16_t sourceY = 0;
  if (x < 0) {
    sourceX = -x;
    width = static_cast<uint16_t>(width + x);
    x = 0;
  }
  if (y < 0) {
    sourceY = -y;
    height = static_cast<uint16_t>(height + y);
    y = 0;
  }
  if (x + width > this->width_) {
    width = static_cast<uint16_t>(this->width_ - x);
  }
  if (y + height > this->height_) {
    height = static_cast<uint16_t>(this->height_ - y);
  }
  if (width == 0 || height == 0) {
    return;
  }

  setAddressWindowUnchecked(static_cast<uint16_t>(x), static_cast<uint16_t>(y),
                            static_cast<uint16_t>(x + width - 1),
                            static_cast<uint16_t>(y + height - 1));

  spi_.beginTransaction(settings_);
  digitalWrite(kCsPin, LOW);
  digitalWrite(kDcPin, HIGH);
  for (uint16_t row = 0; row < height; ++row) {
    const uint16_t* sourceRow =
        pixels + static_cast<size_t>(sourceY + row) * sourceWidth + sourceX;
    for (uint16_t column = 0; column < width; ++column) {
      const uint16_t color = sourceRow[column];
      spi_.transfer(static_cast<uint8_t>(color >> 8));
      spi_.transfer(static_cast<uint8_t>(color));
    }
  }
  digitalWrite(kCsPin, HIGH);
  spi_.endTransaction();
}

void Display::drawPixel(int16_t x, int16_t y, uint16_t color) {
  if (!initialized_ || x < 0 || y < 0 || x >= width_ || y >= height_) {
    return;
  }
  setAddressWindowUnchecked(static_cast<uint16_t>(x), static_cast<uint16_t>(y),
                            static_cast<uint16_t>(x), static_cast<uint16_t>(y));
  writeColor(color, 1);
}

void Display::drawFastHLine(int16_t x, int16_t y, int16_t width,
                            uint16_t color) {
  if (!initialized_ || y < 0 || y >= height_ || width <= 0) {
    return;
  }
  if (x < 0) {
    width += x;
    x = 0;
  }
  if (x + width > this->width_) {
    width = this->width_ - x;
  }
  if (width <= 0) {
    return;
  }
  setAddressWindowUnchecked(static_cast<uint16_t>(x), static_cast<uint16_t>(y),
                            static_cast<uint16_t>(x + width - 1),
                            static_cast<uint16_t>(y));
  writeColor(color, static_cast<uint32_t>(width));
}

void Display::drawFastVLine(int16_t x, int16_t y, int16_t height,
                            uint16_t color) {
  if (!initialized_ || x < 0 || x >= width_ || height <= 0) {
    return;
  }
  if (y < 0) {
    height += y;
    y = 0;
  }
  if (y + height > this->height_) {
    height = this->height_ - y;
  }
  if (height <= 0) {
    return;
  }
  setAddressWindowUnchecked(static_cast<uint16_t>(x), static_cast<uint16_t>(y),
                            static_cast<uint16_t>(x),
                            static_cast<uint16_t>(y + height - 1));
  writeColor(color, static_cast<uint32_t>(height));
}

void Display::drawRect(int16_t x, int16_t y, int16_t width, int16_t height,
                       uint16_t color) {
  if (width <= 0 || height <= 0) return;
  drawFastHLine(x, y, width, color);
  drawFastHLine(x, y + height - 1, width, color);
  drawFastVLine(x, y, height, color);
  drawFastVLine(x + width - 1, y, height, color);
}

uint16_t Display::width() const { return width_; }
uint16_t Display::height() const { return height_; }
bool Display::initialized() const { return initialized_; }

Display lcd;

}  //  OmiPetDisplay 命名空间 / OmiPetDisplay namespace
