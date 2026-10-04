#include "led_controller.h"

#include <algorithm>
#include <cmath>

namespace OmiPetLed {
namespace {

//  所有动画参数集中放在这里，便于调节速度、柔和度和最低亮度 / Keep animation tuning parameters together for speed, softness, and minimum brightness
//  每 16 ms 最多发送一帧，约等于 60 FPS；动画不依赖 loop() 执行次数 / Send at most one frame every 16 ms, about 60 FPS; animation does not depend on loop count
constexpr uint32_t kFrameIntervalMs = 16;
//  彩虹颜色完成一整圈所需的时间 / Time required for the rainbow hue to complete one full cycle
constexpr uint32_t kRainbowCycleMs = 7200;
//  呼吸灯从暗到亮再回到暗的完整周期 / Full breathing cycle from dim to bright and back to dim
constexpr uint32_t kBreatheCycleMs = 3000;
//  光点从最左移动到最右再返回最左的完整周期 / Full sweep cycle from the left end to the right and back
constexpr uint32_t kSweepCycleMs = 3600;
//  光环从中心扩散到两侧再收回中心的完整周期 / Full cycle for the ring to expand from center and return
constexpr uint32_t kCenterExpandCycleMs = 4200;
//  余弦函数的数学常量 / Mathematical constants used by cosine easing
constexpr float kPi = 3.14159265358979323846F;
constexpr float kTwoPi = 2.0F * kPi;
//  保留极低的基础亮度，让非焦点灯珠不会完全突兀熄灭 / Keep a very low ambient level so non-focus pixels do not turn off abruptly
constexpr uint8_t kAmbientLevel = 7;

//  将数值限制到 0.0 到 1.0，避免浮点误差导致亮度越界 / Clamp a value to 0.0 through 1.0 to prevent brightness overflow
float clampUnit(float value) {
  return std::max(0.0F, std::min(value, 1.0F));
}

//  根据距离计算柔和光晕：中心距离为 0 时最亮，超过半径时为 0 / Calculate a soft glow: brightest at distance 0 and zero beyond the radius
//  余弦曲线在边缘平滑收敛，比线性截断更适合连续灯效 / Cosine easing converges smoothly at the edge and avoids harsh linear cutoffs
float cosineFalloff(float distance, float radius) {
  if (radius <= 0.0F) {
    return 0.0F;
  }
  const float normalizedDistance = clampUnit(distance / radius);
  return 0.5F * (1.0F + std::cos(kPi * normalizedDistance));
}

//  把 0.0 到 1.0 的光晕强度映射到 LED 的 0 到 255 亮度 / Map glow intensity from 0.0 through 1.0 to LED level 0 through 255
//  先加入基础亮度，再四舍五入为整数，减少闪烁和截断误差 / Add ambient brightness first, then round to reduce flicker and truncation error
uint8_t levelFromGlow(float glow) {
  const float level = static_cast<float>(kAmbientLevel) +
                      clampUnit(glow) * (255.0F - kAmbientLevel);
  return static_cast<uint8_t>(std::lround(level));
}

//  按亮度比例缩放一个 RGB 分量，不改变当前动画的颜色色相 / Scale one RGB component without changing the animation's selected hue
uint8_t scaleComponent(uint8_t component, uint8_t level) {
  return static_cast<uint8_t>(
      (static_cast<uint16_t>(component) * level) / 255U);
}

}  //  匿名命名空间 / Anonymous namespace

//  创建底层 NeoPixel 对象：13 颗灯、GPIO47、GRB 顺序、800 kHz 协议 / Create the NeoPixel object: 13 LEDs, GPIO47, GRB order, and 800 kHz protocol
Strip::Strip() : pixels_(kLedCount, kDataPin, NEO_GRB + NEO_KHZ800) {}

void Strip::begin(uint8_t brightness) {
  //  初始化 NeoPixel 外设并设置全局亮度；全局亮度由库在发送时统一缩放 / Initialize NeoPixel and set global brightness; the library scales output during transmission
  pixels_.begin();
  setBrightness(brightness);
  //  上电先清空缓存并发送黑帧，避免灯带保留上一次的随机状态 / Clear and transmit a black frame at boot to avoid stale or random LED state
  clear(false);
  show();
}

void Strip::clear(bool update) {
  //  clear() 只修改 RAM 缓存；update=true 时才立即把黑帧发送到灯带 / clear() changes only the RAM buffer; update=true also immediately sends the black frame
  pixels_.clear();
  if (update) {
    show();
  }
}

void Strip::show() {
  //  将当前缓存一次性发送给 WS2812B；动画每帧只在这里真正更新硬件 / Transmit the complete buffer to WS2812B; this is the only hardware update per animation frame
  pixels_.show();
}

void Strip::setBrightness(uint8_t brightness) {
  //  这是整条灯带的全局亮度，不会改变动画保存的 RGB 颜色 / This is strip-wide brightness and does not change the RGB color stored by the animation
  brightness_ = brightness;
  pixels_.setBrightness(brightness_);
}

uint8_t Strip::brightness() const { return brightness_; }

void Strip::setPixel(uint16_t index, uint8_t red, uint8_t green,
                     uint8_t blue) {
  //  先写入缓存，不在单颗灯珠更新时发送；调用方完成整帧后再 show() / Write the buffer without transmitting; the caller calls show() after building the whole frame
  if (index < kLedCount) {
    pixels_.setPixelColor(index, pixels_.Color(red, green, blue));
  }
}

void Strip::setPixel(uint16_t index, uint32_t colorValue) {
  //  允许动画直接写入 NeoPixel 打包颜色 / Allow effects to write a packed NeoPixel color directly
  if (index < kLedCount) {
    pixels_.setPixelColor(index, colorValue);
  }
}

void Strip::fill(uint8_t red, uint8_t green, uint8_t blue, bool update) {
  //  RGB 版本先转换为库使用的打包颜色，再复用统一的填充实现 / Convert RGB to the library's packed color and reuse the common fill implementation
  fill(color(red, green, blue), update);
}

void Strip::fill(uint32_t colorValue, bool update) {
  //  一次填满全部 LED；动画需要逐颗渐变时使用 setPixel() / Fill all LEDs at once; use setPixel() when per-pixel gradients are needed
  pixels_.fill(colorValue, 0, kLedCount);
  if (update) {
    show();
  }
}

void Strip::setPixelHSV(uint16_t index, uint8_t hue, uint8_t saturation,
                        uint8_t value) {
  //  HSV 更适合彩虹动画：只改变 hue 就能连续移动色相 / HSV suits rainbow animation because changing hue alone moves the color continuously
  if (index < kLedCount) {
    pixels_.setPixelColor(index, pixels_.ColorHSV(
        static_cast<uint16_t>(hue) * 256U, saturation, value));
  }
}

uint32_t Strip::color(uint8_t red, uint8_t green, uint8_t blue) const {
  //  使用 Adafruit_NeoPixel 的颜色打包方式，避免手动处理 GRB 顺序 / Use Adafruit_NeoPixel packing to avoid manually handling GRB byte order
  return pixels_.Color(red, green, blue);
}

uint32_t Strip::colorWheel(uint8_t position) const {
  //  把 0 到 255 的位置映射为红、绿、蓝连续过渡的颜色 / Map position 0 through 255 to a continuous red-green-blue color transition
  position = 255 - position;
  if (position < 85) {
    return color(255 - position * 3, 0, position * 3);
  }
  if (position < 170) {
    position -= 85;
    return color(0, position * 3, 255 - position * 3);
  }
  position -= 170;
  return color(position * 3, 255 - position * 3, 0);
}

//  动画控制器只保存状态，并通过引用操作 Strip，不直接拥有硬件对象 / The effect controller stores state and operates on Strip by reference without owning hardware
EffectController::EffectController(Strip& strip) : strip_(strip) {}

void EffectController::begin() {
  //  记录初始化时间；之后每个模式都用相对时间计算位置和亮度 / Record initialization time; each mode uses relative time for position and brightness
  initialized_ = true;
  modeStartedAtMs_ = millis();
  lastFrameAtMs_ = 0;
}

void EffectController::update() {
  //  常亮和关闭没有时间变化，不需要每次 loop() 重复发送 / Solid and off modes do not change over time and need no repeated loop transmission
  if (!initialized_ || mode_ == EffectMode::Solid || mode_ == EffectMode::Off) {
    return;
  }
  //  主循环只负责调用 update()；具体帧率和渲染由 render() 控制 / The main loop only calls update(); render() controls frame timing and drawing
  render(millis(), false);
}

void EffectController::setSolid(uint8_t red, uint8_t green, uint8_t blue) {
  //  保存基础颜色；呼吸、扫描和扩散都以这组 RGB 为颜色来源 / Store the base color used by breathing, sweep, and expansion effects
  red_ = red;
  green_ = green;
  blue_ = blue;
  setSolid();
}

void EffectController::setSolid() {
  //  切换模式后立即绘制一帧，让语音命令无需等待下一次 loop() / Render immediately after switching so a voice command does not wait for the next loop
  selectMode(EffectMode::Solid);
  render(millis(), true);
}

void EffectController::setRainbow() {
  //  彩虹模式沿用当前亮度，但每颗灯使用不同的色相偏移 / Rainbow mode uses current brightness with a hue offset for each LED
  selectMode(EffectMode::Rainbow);
  render(millis(), true);
}

void EffectController::setBreathe() {
  //  呼吸模式沿用当前颜色，只随时间改变整体亮度 / Breathing mode keeps the current color and changes only overall brightness over time
  selectMode(EffectMode::Breathe);
  render(millis(), true);
}

void EffectController::setSweep() {
  //  扫描模式创建一个从左到右、再从右到左移动的柔和光点 / Sweep mode creates a soft light point moving left-to-right and back
  selectMode(EffectMode::Sweep);
  render(millis(), true);
}

void EffectController::setCenterExpand() {
  //  中心扩散模式创建一个从中心向两侧展开、再收回的光环 / Center-expand mode creates a ring expanding from center to both sides and returning
  selectMode(EffectMode::CenterExpand);
  render(millis(), true);
}

void EffectController::setOff() {
  //  关闭模式会清空缓存并发送黑帧 / Off mode clears the buffer and sends a black frame
  selectMode(EffectMode::Off);
  render(millis(), true);
}

EffectMode EffectController::mode() const { return mode_; }

void EffectController::selectMode(EffectMode mode) {
  //  每次切换模式都从 0 ms 重新开始，避免新动画接着旧动画的半途位置运行 / Restart each timeline at 0 ms so a new effect does not inherit the old effect's midpoint
  mode_ = mode;
  modeStartedAtMs_ = millis();
  lastFrameAtMs_ = 0;
}

void EffectController::render(uint32_t nowMs, bool force) {
  //  非强制渲染遵守 16 ms 帧间隔；无论 loop() 多快，都不会过度发送数据 / Non-forced rendering obeys the 16 ms interval and avoids excessive bus traffic regardless of loop speed
  if (!force && nowMs - lastFrameAtMs_ < kFrameIntervalMs) {
    return;
  }
  lastFrameAtMs_ = nowMs;
  //  使用无符号时间差，millis() 溢出时仍能正确工作 / Use unsigned elapsed time so the logic remains valid across millis() rollover
  const uint32_t elapsedMs = nowMs - modeStartedAtMs_;

  //  统一分派到当前模式的渲染函数，保持硬件层和动画层分离 / Dispatch to the active renderer while keeping hardware and animation layers separate
  switch (mode_) {
    case EffectMode::Solid:
      renderSolid();
      break;
    case EffectMode::Rainbow:
      renderRainbow(elapsedMs);
      break;
    case EffectMode::Breathe:
      renderBreathe(elapsedMs);
      break;
    case EffectMode::Sweep:
      renderSweep(elapsedMs);
      break;
    case EffectMode::CenterExpand:
      renderCenterExpand(elapsedMs);
      break;
    case EffectMode::Off:
      renderOff();
      break;
  }
}

void EffectController::renderSolid() {
  //  常亮模式把当前 RGB 写入整条缓存并发送 / Fill the complete buffer with the current RGB and transmit it
  strip_.fill(red_, green_, blue_);
}

void EffectController::renderRainbow(uint32_t elapsedMs) {
  //  将经过时间折算为 0.0 到 1.0 的循环相位 / Convert elapsed time into a looping phase from 0.0 through 1.0
  const float phase = static_cast<float>(elapsedMs % kRainbowCycleMs) /
                      static_cast<float>(kRainbowCycleMs);
  //  基础色相随时间移动；每颗灯再叠加空间偏移，形成流动彩虹 / Move the base hue over time and add a spatial offset to create a flowing rainbow
  const uint8_t baseHue = static_cast<uint8_t>(phase * 256.0F);
  for (uint16_t index = 0; index < kLedCount; ++index) {
    //  按灯珠位置均匀分布色相，避免 13 颗灯显示成同一种颜色 / Distribute hue evenly across the 13 LEDs instead of showing one color everywhere
    const uint8_t offset = static_cast<uint8_t>(
        (static_cast<uint16_t>(index) * 256U) / kLedCount);
    strip_.setPixelHSV(index, static_cast<uint8_t>(baseHue + offset), 255, 255);
  }
  strip_.show();
}

void EffectController::renderBreathe(uint32_t elapsedMs) {
  //  相位从 0 到 1 循环；余弦波让亮度在最高点和最低点都平滑停留 / Loop phase from 0 to 1; cosine keeps brightness smooth at both extremes
  const float phase = static_cast<float>(elapsedMs % kBreatheCycleMs) /
                      static_cast<float>(kBreatheCycleMs);
  //  计算整体亮度波形，并保留少量基础亮度 / Calculate overall brightness and retain a small ambient level
  const float wave = 0.5F - 0.5F * std::cos(kTwoPi * phase);
  const uint8_t level = levelFromGlow(0.04F + wave * 0.96F);
  strip_.fill(scaleComponent(red_, level), scaleComponent(green_, level),
              scaleComponent(blue_, level));
}

void EffectController::renderSweep(uint32_t elapsedMs) {
  //  把时间转换为往返相位：0 和 1 都在最左端，0.5 在最右端 / Convert time to a ping-pong phase: 0 and 1 are left, while 0.5 is right
  const float phase = static_cast<float>(elapsedMs % kSweepCycleMs) /
                      static_cast<float>(kSweepCycleMs);
  //  余弦位置让光点到达端点时自然减速并反向，不会突然折返 / Cosine position eases at both ends before reversing instead of snapping
  const float position = static_cast<float>(kLedCount - 1U) * 0.5F *
                         (1.0F - std::cos(kTwoPi * phase));
  for (uint16_t index = 0; index < kLedCount; ++index) {
    //  当前灯珠距离光点越近越亮，距离越远越暗 / LEDs closer to the moving point are brighter, and farther LEDs are dimmer
    const float distance = std::fabs(static_cast<float>(index) - position);
    //  core 控制亮斑，halo 扩大柔和范围；两者取较大值避免出现断层 / core controls the highlight and halo widens the softness; max avoids visible gaps
    const float core = cosineFalloff(distance, 1.8F);
    const float halo = cosineFalloff(distance, 4.6F) * 0.34F;
    const uint8_t level = levelFromGlow(std::max(core, halo));
    strip_.setPixel(index, scaleComponent(red_, level),
                    scaleComponent(green_, level), scaleComponent(blue_, level));
  }
  strip_.show();
}

void EffectController::renderCenterExpand(uint32_t elapsedMs) {
  //  计算中心对称的动画相位，中心和最外侧分别对应半径 0 和最大半径 / Calculate a center-symmetric phase where radius 0 is center and maximum radius is the edge
  const float phase = static_cast<float>(elapsedMs % kCenterExpandCycleMs) /
                      static_cast<float>(kCenterExpandCycleMs);
  //  13 颗灯的几何中心位于索引 6；最大半径约为 6 个灯珠间距 / The geometric center of 13 LEDs is index 6; maximum radius is about six LED spacings
  const float maximumRadius = static_cast<float>(kLedCount - 1U) * 0.5F;
  //  余弦半径从中心平滑扩展到边缘，再平滑回到中心 / Cosine radius smoothly expands to the edge and returns to center
  const float radius = maximumRadius * 0.5F * (1.0F - std::cos(kTwoPi * phase));
  const float center = maximumRadius;
  for (uint16_t index = 0; index < kLedCount; ++index) {
    //  用灯珠距中心的距离与当前光环半径比较，得到光环厚度方向的距离 / Compare each LED's center distance with the ring radius to get distance from the ring
    const float distanceFromCenter =
        std::fabs(static_cast<float>(index) - center);
    const float ringDistance = std::fabs(distanceFromCenter - radius);
    //  同时叠加核心光环和外围光晕，避免扩散边缘生硬 / Combine a bright ring with a wider halo for a soft expansion edge
    const float core = cosineFalloff(ringDistance, 1.25F);
    const float halo = cosineFalloff(ringDistance, 3.8F) * 0.34F;
    const uint8_t level = levelFromGlow(std::max(core, halo));
    strip_.setPixel(index, scaleComponent(red_, level),
                    scaleComponent(green_, level), scaleComponent(blue_, level));
  }
  strip_.show();
}

void EffectController::renderOff() {
  //  先清理缓存，再发送黑帧，确保灯珠实际熄灭 / Clear the buffer first, then transmit a black frame so the LEDs actually turn off
  strip_.clear(false);
  strip_.show();
}

//  全局硬件对象和动画控制器；控制器引用 strip，不复制或重新拥有硬件缓存 / Global hardware object and effect controller; the controller references strip without copying or owning its buffer
Strip strip;
EffectController effects(strip);

//  暴露底层对象给需要读取 NeoPixel 状态的调试代码 / Expose the underlying object to diagnostic code that needs NeoPixel state
Adafruit_NeoPixel& Strip::pixels() { return pixels_; }
const Adafruit_NeoPixel& Strip::pixels() const { return pixels_; }

}  //  OmiPetLed 命名空间 / OmiPetLed namespace
