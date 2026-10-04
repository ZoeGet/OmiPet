#include "buzzer.h"

//  无源蜂鸣器的 LEDC 音调驱动实现 / LEDC tone-driver implementation for the passive buzzer
namespace OmiPetBuzzer {
namespace {

//  10-bit LEDC 分辨率对应的最大占空比 / Maximum duty value for 10-bit LEDC resolution
constexpr uint32_t kPwmMaxDuty = (1UL << kBuzzerPwmResolutionBits) - 1UL;  //  当前 PWM 分辨率下的最大占空比计数值 / Maximum duty-count value for the configured PWM resolution

}  //  匿名命名空间 / Anonymous namespace

//  初始化蜂鸣器 GPIO 和 LEDC PWM 通道 / Initialize the buzzer GPIO and LEDC PWM channel
bool PassiveBuzzer::begin(uint8_t pin, uint8_t channel) {
  pin_ = pin;
  channel_ = channel;
  initialized_ = false;
  playing_ = false;
  muted_ = false;
  lastError_ = BuzzerError::None;

  //  先将引脚配置为低电平，避免上电误鸣叫 / Drive the pin low first to prevent a startup chirp
  pinMode(pin_, OUTPUT);
  digitalWrite(pin_, LOW);

  //  配置固定 LEDC 分辨率和初始频率 / Configure a fixed LEDC resolution and initial frequency
  if (ledcSetup(channel_, kBuzzerDefaultFrequencyHz,
                kBuzzerPwmResolutionBits) == 0U) {
    return fail(BuzzerError::PwmSetupFailed);
  }
  ledcAttachPin(pin_, channel_);
  ledcWrite(channel_, 0);
  ledcDetachPin(pin_);
  pinMode(pin_, OUTPUT);
  digitalWrite(pin_, LOW);

  initialized_ = true;
  return true;
}

//  开始播放指定频率和时长的提示音 / Start a tone with the requested frequency and duration
bool PassiveBuzzer::startTone(uint32_t frequencyHz,
                              uint32_t durationMs,
                              uint8_t dutyPercent) {
  if (!initialized_) {
    return fail(BuzzerError::NotInitialized);
  }
  if (frequencyHz == 0U) {
    return fail(BuzzerError::InvalidFrequency);
  }
  if (dutyPercent == 0U || dutyPercent >= 100U) {
    return fail(BuzzerError::InvalidDutyCycle);
  }

  if (!applyTone(frequencyHz, dutyPercent)) {
    return false;
  }

  startedAtMs_ = millis();
  durationMs_ = durationMs;
  frequencyHz_ = frequencyHz;
  dutyPercent_ = dutyPercent;
  playing_ = true;
  lastError_ = BuzzerError::None;
  return true;
}

//  立即停止当前蜂鸣器输出 / Stop the current buzzer output immediately
void PassiveBuzzer::stop() {
  if (!initialized_) {
    return;
  }

  //  先关闭占空比，再解除引脚复用并拉低 / Disable duty first, then detach the pin and drive it low
  ledcWrite(channel_, 0);
  ledcDetachPin(pin_);
  pinMode(pin_, OUTPUT);
  digitalWrite(pin_, LOW);
  playing_ = false;
  durationMs_ = 0;
}

//  检查提示音是否到时，到时后自动停止 / Stop the tone when its duration expires
void PassiveBuzzer::update() {
  if (!playing_ || durationMs_ == 0U) {
    return;
  }
  if (millis() - startedAtMs_ >= durationMs_) {
    stop();
  }
}

//  设置静音状态，并同步关闭或恢复当前音调 / Set mute state and silence or restore the active tone
void PassiveBuzzer::setMute(bool muted) {
  muted_ = muted;
  if (!initialized_ || !playing_) {
    return;
  }

  //  静音只关闭 PWM 占空比，恢复时重新应用原音符 / Mute only disables PWM duty; restore the current tone when unmuted
  if (muted_) {
    ledcWrite(channel_, 0);
  } else {
    applyTone(frequencyHz_, dutyPercent_);
  }
}

//  查询蜂鸣器当前是否正在播放提示音 / Check whether a tone is currently playing
bool PassiveBuzzer::isPlaying() const {
  return playing_;
}

//  查询蜂鸣器是否处于静音状态 / Check whether the buzzer is muted
bool PassiveBuzzer::isMuted() const {
  return muted_;
}

//  返回最近一次蜂鸣器错误码 / Return the most recent buzzer error code
BuzzerError PassiveBuzzer::lastError() const {
  return lastError_;
}

//  把频率和占空比写入 LEDC，真正产生 PWM 输出 / Apply frequency and duty cycle to the LEDC PWM output
bool PassiveBuzzer::applyTone(uint32_t frequencyHz, uint8_t dutyPercent) {
  if (ledcSetup(channel_, frequencyHz, kBuzzerPwmResolutionBits) == 0U) {
    return fail(BuzzerError::PwmSetupFailed);
  }

  //  静音时保持占空比为 0，但仍保留播放计时 / Keep duty at zero while muted, but preserve playback timing
  const uint32_t duty = muted_
                            ? 0U
                            : (kPwmMaxDuty * dutyPercent) / 100U;
  ledcAttachPin(pin_, channel_);
  ledcWrite(channel_, duty);
  return true;
}

//  保存错误码并结束当前播放，统一处理失败路径 / Save an error code and stop the current tone
bool PassiveBuzzer::fail(BuzzerError error) {
  lastError_ = error;
  stop();
  return false;
}

PassiveBuzzer buzzer;

}  //  OmiPetBuzzer 命名空间 / OmiPetBuzzer namespace
