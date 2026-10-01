#include "buzzer.h"

namespace OmiPetBuzzer {
namespace {

//  10-bit LEDC 分辨率对应的最大占空比 / Maximum duty value for 10-bit LEDC resolution
constexpr uint32_t kPwmMaxDuty = (1UL << kBuzzerPwmResolutionBits) - 1UL;

}  //  匿名命名空间 / Anonymous namespace

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

void PassiveBuzzer::update() {
  if (!playing_ || durationMs_ == 0U) {
    return;
  }
  if (millis() - startedAtMs_ >= durationMs_) {
    stop();
  }
}

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

bool PassiveBuzzer::isPlaying() const {
  return playing_;
}

bool PassiveBuzzer::isMuted() const {
  return muted_;
}

BuzzerError PassiveBuzzer::lastError() const {
  return lastError_;
}

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

bool PassiveBuzzer::fail(BuzzerError error) {
  lastError_ = error;
  stop();
  return false;
}

PassiveBuzzer buzzer;

}  //  OmiPetBuzzer 命名空间 / OmiPetBuzzer namespace
