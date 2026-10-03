#include "omi_pet_ui.h"

//  OmiPet 屏幕界面绘制实现 / OmiPet screen user-interface rendering implementation

#include <Arduino.h>

#include <cstring>
#include <cstdio>
#include <cstdint>

#include "NV3007_Display.h"

namespace OmiPetUi {
namespace {

struct Glyph {
  char character;
  uint8_t rows[7];
};

//  简单 5×7 字模 / Simple 5x7 bitmap font
constexpr Glyph kFont[] = {  //  内置 ASCII 5×7 字模表 / Built-in ASCII 5x7 glyph table
    {' ', {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    {'-', {0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00}},
    {':', {0x00, 0x04, 0x04, 0x00, 0x04, 0x04, 0x00}},
    {'.', {0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x06}},
    {'%', {0x19, 0x19, 0x02, 0x04, 0x08, 0x13, 0x13}},
    {'0', {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E}},
    {'1', {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E}},
    {'2', {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F}},
    {'3', {0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E}},
    {'4', {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02}},
    {'5', {0x1F, 0x10, 0x10, 0x1E, 0x01, 0x01, 0x1E}},
    {'6', {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E}},
    {'7', {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08}},
    {'8', {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E}},
    {'9', {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C}},
    {'A', {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}},
    {'B', {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E}},
    {'C', {0x0F, 0x10, 0x10, 0x10, 0x10, 0x10, 0x0F}},
    {'D', {0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E}},
    {'E', {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F}},
    {'F', {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10}},
    {'G', {0x0F, 0x10, 0x10, 0x17, 0x11, 0x11, 0x0F}},
    {'H', {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}},
    {'I', {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x1F}},
    {'J', {0x01, 0x01, 0x01, 0x01, 0x11, 0x11, 0x0E}},
    {'K', {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11}},
    {'L', {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F}},
    {'M', {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11}},
    {'N', {0x11, 0x19, 0x19, 0x15, 0x13, 0x13, 0x11}},
    {'O', {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}},
    {'P', {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10}},
    {'Q', {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D}},
    {'R', {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11}},
    {'S', {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E}},
    {'T', {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}},
    {'U', {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}},
    {'V', {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04}},
    {'W', {0x11, 0x11, 0x11, 0x15, 0x15, 0x1B, 0x11}},
    {'X', {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11}},
    {'Y', {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04}},
    {'Z', {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F}},
};

constexpr uint16_t kBackground =  //  UI 背景色 / UI background color
    OmiPetDisplay::Display::color565(5, 12, 28);
constexpr uint16_t kAccent = OmiPetDisplay::Display::color565(72, 220, 255);  //  UI 强调色 / UI accent color
constexpr uint16_t kPetColor = OmiPetDisplay::Display::color565(255, 180, 80);  //  宠物主体颜色 / Pet body color
constexpr uint16_t kWhite = OmiPetDisplay::Display::color565(245, 248, 255);  //  UI 白色文字颜色 / UI white text color
constexpr uint16_t kGreen = OmiPetDisplay::Display::color565(100, 235, 150);  //  正常状态颜色 / Normal-status color
constexpr uint16_t kYellow = OmiPetDisplay::Display::color565(255, 220, 80);  //  提示状态颜色 / Attention-status color
constexpr uint16_t kRed = OmiPetDisplay::Display::color565(255, 90, 90);  //  错误状态颜色 / Error-status color
constexpr uint16_t kDark = OmiPetDisplay::Display::color565(5, 12, 28);  //  深色填充颜色 / Dark fill color

constexpr int16_t kScreenWidth = OmiPetDisplay::kPanelWidth;  //  屏幕面板宽度 / Display-panel width

uint32_t gClockStartMillis = 0;
uint32_t gClockBaseSeconds = 0;
uint32_t gLastRenderedSecond = UINT32_MAX;
bool gBlink = false;
bool gUiStarted = false;
bool gEnvironmentValid = false;
float gTemperatureC = 0.0F;
float gHumidityPercent = 0.0F;
bool gNetworkConnected = false;
bool gNetworkProvisioning = false;
const char* gVoiceStatus = "IDLE";
uint16_t gGlyphBitmap[15 * 21] = {};

//  查找内置字模 / Find a glyph in the built-in font
const Glyph* findGlyph(char character) {
  if (character >= 'a' && character <= 'z') {
    character = static_cast<char>(character - ('a' - 'A'));
  }
  for (const Glyph& glyph : kFont) {
    if (glyph.character == character) {
      return &glyph;
    }
  }
  return &kFont[0];
}

int16_t textWidth(const char* text, uint8_t scale) {
  const size_t length = std::strlen(text);
  if (length == 0) {
    return 0;
  }
  return static_cast<int16_t>((length * 6U - 1U) * scale);
}

//  绘制左对齐文本 / Draw left-aligned text
void drawText(int16_t x, int16_t y, const char* text, uint8_t scale,
              uint16_t color) {
  const uint16_t glyphWidth = static_cast<uint16_t>(5U * scale);
  const uint16_t glyphHeight = static_cast<uint16_t>(7U * scale);
  for (size_t characterIndex = 0; text[characterIndex] != '\0';
       ++characterIndex) {
    const Glyph* glyph = findGlyph(text[characterIndex]);
    for (uint8_t row = 0; row < 7; ++row) {
      for (uint8_t column = 0; column < 5; ++column) {
        const uint16_t pixelColor =
            (glyph->rows[row] & (0x10U >> column)) != 0 ? color : kBackground;
        for (uint8_t scaleY = 0; scaleY < scale; ++scaleY) {
          for (uint8_t scaleX = 0; scaleX < scale; ++scaleX) {
            const uint16_t bitmapX =
                static_cast<uint16_t>(column * scale + scaleX);
            const uint16_t bitmapY =
                static_cast<uint16_t>(row * scale + scaleY);
            gGlyphBitmap[bitmapY * glyphWidth + bitmapX] = pixelColor;
          }
        }
      }
    }
    OmiPetDisplay::lcd.drawBitmap(x, y, glyphWidth, glyphHeight,
                                  gGlyphBitmap);
    x += static_cast<int16_t>(6 * scale);
  }
}

void drawCenteredText(int16_t y, const char* text, uint8_t scale,
                      uint16_t color) {
  const int16_t width = textWidth(text, scale);
  drawText((kScreenWidth - width) / 2, y, text, scale, color);
}

uint32_t compileTimeSeconds() {
  const char* time = __TIME__;
  const uint32_t hours = static_cast<uint32_t>(time[0] - '0') * 10U +
                         static_cast<uint32_t>(time[1] - '0');
  const uint32_t minutes = static_cast<uint32_t>(time[3] - '0') * 10U +
                           static_cast<uint32_t>(time[4] - '0');
  const uint32_t seconds = static_cast<uint32_t>(time[6] - '0') * 10U +
                           static_cast<uint32_t>(time[7] - '0');
  return hours * 3600U + minutes * 60U + seconds;
}

uint32_t currentClockSeconds() {
  const uint32_t elapsedSeconds = (millis() - gClockStartMillis) / 1000U;
  return (gClockBaseSeconds + elapsedSeconds) % (24U * 60U * 60U);
}

void drawPet(bool blink) {
  OmiPetDisplay::lcd.fillRect(12, 42, 118, 132, kBackground);

  //  耳朵和脸部 / Ears and face
  OmiPetDisplay::lcd.fillRect(31, 48, 22, 20, kPetColor);
  OmiPetDisplay::lcd.fillRect(89, 48, 22, 20, kPetColor);
  OmiPetDisplay::lcd.fillRect(24, 60, 94, 94, kPetColor);
  OmiPetDisplay::lcd.drawRect(24, 60, 94, 94, kAccent);

  //  眼睛和嘴巴 / Eyes and mouth
  if (blink) {
    OmiPetDisplay::lcd.fillRect(40, 88, 20, 4, kDark);
    OmiPetDisplay::lcd.fillRect(82, 88, 20, 4, kDark);
  } else {
    OmiPetDisplay::lcd.fillRect(40, 80, 20, 24, kWhite);
    OmiPetDisplay::lcd.fillRect(82, 80, 20, 24, kWhite);
    OmiPetDisplay::lcd.fillRect(47, 88, 7, 10, kDark);
    OmiPetDisplay::lcd.fillRect(89, 88, 7, 10, kDark);
  }
  OmiPetDisplay::lcd.fillRect(60, 119, 22, 4, kDark);
  OmiPetDisplay::lcd.fillRect(68, 123, 6, 4, kDark);
  OmiPetDisplay::lcd.fillRect(33, 116, 10, 5, kAccent);
  OmiPetDisplay::lcd.fillRect(99, 116, 10, 5, kAccent);
}

void drawPetEyes(bool blink) {
  //  只刷新眼睛区域，避免眨眼时重绘整张脸 / Refresh only the eye areas to avoid redrawing the whole face
  OmiPetDisplay::lcd.fillRect(40, 76, 20, 32, kPetColor);
  OmiPetDisplay::lcd.fillRect(82, 76, 20, 32, kPetColor);
  if (blink) {
    OmiPetDisplay::lcd.fillRect(40, 88, 20, 4, kDark);
    OmiPetDisplay::lcd.fillRect(82, 88, 20, 4, kDark);
    return;
  }
  OmiPetDisplay::lcd.fillRect(40, 80, 20, 24, kWhite);
  OmiPetDisplay::lcd.fillRect(82, 80, 20, 24, kWhite);
  OmiPetDisplay::lcd.fillRect(47, 88, 7, 10, kDark);
  OmiPetDisplay::lcd.fillRect(89, 88, 7, 10, kDark);
}

void drawClock(uint32_t seconds) {
  char clockText[12] = {};
  const uint32_t hours = seconds / 3600U;
  const uint32_t minutes = (seconds / 60U) % 60U;
  std::snprintf(clockText, sizeof(clockText), "%02lu:%02lu",
                static_cast<unsigned long>(hours),
                static_cast<unsigned long>(minutes));

  drawCenteredText(205, clockText, 3, kWhite);
}

void formatTemperature(char* output, size_t outputSize) {
  int32_t tenths = static_cast<int32_t>(gTemperatureC * 10.0F +
                                        (gTemperatureC >= 0.0F ? 0.5F : -0.5F));
  const bool negative = tenths < 0;
  const uint32_t absoluteTenths =
      static_cast<uint32_t>(negative ? -tenths : tenths);
  const uint32_t whole = absoluteTenths / 10U;
  const uint32_t fraction = absoluteTenths % 10U;
  if (negative) {
    std::snprintf(output, outputSize, "T -%lu.%luC",
                  static_cast<unsigned long>(whole),
                  static_cast<unsigned long>(fraction));
  } else {
    std::snprintf(output, outputSize, "T %lu.%luC",
                  static_cast<unsigned long>(whole),
                  static_cast<unsigned long>(fraction));
  }
}

void drawEnvironment() {
  char temperatureText[12] = {};
  char humidityText[10] = {};
  if (gEnvironmentValid) {
    formatTemperature(temperatureText, sizeof(temperatureText));
    const int32_t humidity = static_cast<int32_t>(gHumidityPercent + 0.5F);
    std::snprintf(humidityText, sizeof(humidityText), "H %ld%%",
                  static_cast<long>(humidity));
  } else {
    std::strncpy(temperatureText, "T --C", sizeof(temperatureText) - 1U);
    std::strncpy(humidityText, "H --%", sizeof(humidityText) - 1U);
  }

  //  分行局部刷新，避免覆盖整块屏幕 / Refresh each row locally to avoid redrawing the whole screen
  OmiPetDisplay::lcd.fillRect(0, 300, kScreenWidth, 26, kBackground);
  OmiPetDisplay::lcd.fillRect(0, 330, kScreenWidth, 26, kBackground);
  drawCenteredText(304, temperatureText, 2, kGreen);
  drawCenteredText(334, humidityText, 2, kAccent);
}

void drawNetworkStatus() {
  const char* statusText = "WIFI --";
  uint16_t statusColor = kRed;
  if (gNetworkConnected) {
    statusText = "WIFI OK";
    statusColor = kGreen;
  } else if (gNetworkProvisioning) {
    statusText = "WIFI SET";
    statusColor = kYellow;
  }

  //  局部刷新网络状态，避免每次循环重复刷屏 / Refresh only the network status area to avoid repeated full updates
  OmiPetDisplay::lcd.fillRect(0, 400, kScreenWidth, 28, kBackground);
  drawCenteredText(405, statusText, 1, statusColor);
}

void drawVoiceStatus() {
  //  局部刷新语音状态区域 / Refresh only the voice status area
  OmiPetDisplay::lcd.fillRect(0, 382, kScreenWidth, 18, kBackground);
  drawCenteredText(383, gVoiceStatus, 1, kYellow);
}
//  绘制不随时间变化的界面元素 / Draw static user-interface elements
void drawStaticUi() {
  OmiPetDisplay::lcd.fillScreen(kBackground);
  drawCenteredText(10, "OMIPET", 2, kAccent);
  drawPet(false);
  drawClock(currentClockSeconds());
  drawCenteredText(254, __DATE__, 1, kAccent);

  drawEnvironment();
  drawCenteredText(364, "BAT --%", 2, kYellow);
  drawVoiceStatus();
  drawNetworkStatus();
}

}  //  匿名命名空间 / Anonymous namespace

//  初始化屏幕界面和时钟基准 / Initialize the screen UI and clock base
void begin() {
  gClockStartMillis = millis();
  gClockBaseSeconds = compileTimeSeconds();
  gLastRenderedSecond = UINT32_MAX;
  gBlink = false;
  drawStaticUi();
  gUiStarted = true;
}

void setEnvironment(float temperatureC, float humidityPercent, bool valid) {
  gTemperatureC = temperatureC;
  gHumidityPercent = humidityPercent;
  gEnvironmentValid = valid;
  if (gUiStarted) {
    drawEnvironment();
  }
}

void setNetworkStatus(bool connected, bool provisioning) {
  if (gNetworkConnected == connected &&
      gNetworkProvisioning == provisioning) {
    return;
  }

  gNetworkConnected = connected;
  gNetworkProvisioning = provisioning;
  if (gUiStarted) {
    drawNetworkStatus();
  }
}

void setVoiceStatus(const char* statusText) {
  if (statusText == nullptr || std::strcmp(gVoiceStatus, statusText) == 0) {
    return;
  }

  gVoiceStatus = statusText;
  if (gUiStarted) {
    drawVoiceStatus();
  }
}
//  按需刷新动态界面 / Refresh dynamic interface elements when needed
void update() {
  const uint32_t seconds = currentClockSeconds();
  if (seconds == gLastRenderedSecond) {
    return;
  }
  gLastRenderedSecond = seconds;
  drawClock(seconds);

  const bool blink = (seconds % 8U == 0U) || (seconds % 8U == 1U);
  if (blink != gBlink) {
    gBlink = blink;
    drawPetEyes(gBlink);
  }
}

}  //  OmiPetUi 命名空间 / OmiPetUi namespace
