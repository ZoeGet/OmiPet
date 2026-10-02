#include "ics43434_mic.h"

//  ICS-43434 I2S 数字麦克风驱动实现 / ICS-43434 I2S digital microphone driver implementation
#include <algorithm>
#include <cstdint>
#include <limits>

namespace OmiPetAudio {

bool Ics43434Mic::begin(uint32_t sampleRateHz, MicChannel channel) {
  if (sampleRateHz == 0U) {
    return false;
  }

  if (initialized_) {
    end();
  }

  sampleRateHz_ = sampleRateHz;
  channel_ = channel;

  //  显式配置旧版 ESP-IDF I2S 接收驱动 / Configure the legacy ESP-IDF I2S RX driver explicitly
  //  避免 Arduino I2SClass 默认引脚和全局回调队列问题 / This avoids Arduino I2SClass default pins and its global callback queue
  i2s_config_t config = {};
  config.mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_RX);
  config.sample_rate = sampleRateHz_;
  config.bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT;
  config.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
  config.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  config.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
  config.dma_buf_count = 4;
  config.dma_buf_len = 128;
  config.use_apll = false;
  config.tx_desc_auto_clear = false;
  config.fixed_mclk = 0;

  const esp_err_t installResult =
      i2s_driver_install(port_, &config, 0, nullptr);
  if (installResult != ESP_OK) {
    initialized_ = false;
    return false;
  }

  i2s_pin_config_t pins = {};
  pins.mck_io_num = I2S_PIN_NO_CHANGE;
  pins.bck_io_num = kMicSckPin;
  pins.ws_io_num = kMicWsPin;
  pins.data_out_num = I2S_PIN_NO_CHANGE;
  pins.data_in_num = kMicSdPin;

  const esp_err_t pinResult = i2s_set_pin(port_, &pins);
  if (pinResult != ESP_OK) {
    i2s_driver_uninstall(port_);
    initialized_ = false;
    return false;
  }

  initialized_ = true;
  return true;
}

void Ics43434Mic::end() {
  if (!initialized_) {
    return;
  }

  i2s_driver_uninstall(port_);
  initialized_ = false;
}

//  查询 I2S 接收是否已经初始化 / Check whether I2S reception is initialized
bool Ics43434Mic::initialized() const {
  return initialized_;
}

size_t Ics43434Mic::availableRawWords() {
  if (!initialized_) {
    return 0;
  }

  return 0;
}

size_t Ics43434Mic::readRawWords(int32_t* buffer, size_t wordCount) {
  if (!initialized_ || buffer == nullptr || wordCount == 0U) {
    return 0;
  }

  const size_t requestedBytes = wordCount * sizeof(int32_t);
  size_t bytesRead = 0;
  const esp_err_t result =
      i2s_read(port_, buffer, requestedBytes, &bytesRead, pdMS_TO_TICKS(20));
  if (result != ESP_OK || bytesRead == 0U) {
    return 0;
  }

  return std::min(bytesRead / sizeof(int32_t), wordCount);
}

size_t Ics43434Mic::readSelectedFrames(int32_t* buffer, size_t frameCount) {
  if (!initialized_ || buffer == nullptr || frameCount == 0U) {
    return 0;
  }

  size_t framesRead = 0;
  int32_t stereoFrame[2] = {};
  const size_t selectedSlot = static_cast<size_t>(channel_);
  while (framesRead < frameCount) {
    if (readRawWords(stereoFrame, 2U) != 2U) {
      break;
    }
    buffer[framesRead++] = stereoFrame[selectedSlot];
  }

  return framesRead;
}

MicChannel Ics43434Mic::channel() const {
  return channel_;
}

uint32_t Ics43434Mic::sampleRateHz() const {
  return sampleRateHz_;
}

RawSampleStats analyzeRawSamples(const int32_t* samples, size_t sampleCount) {
  RawSampleStats stats;
  if (samples == nullptr || sampleCount == 0U) {
    return stats;
  }

  stats.minimum = std::numeric_limits<int32_t>::max();
  stats.maximum = std::numeric_limits<int32_t>::min();
  stats.sampleCount = sampleCount;

  for (size_t index = 0; index < sampleCount; ++index) {
    const int32_t sample = samples[index];
    stats.minimum = std::min(stats.minimum, sample);
    stats.maximum = std::max(stats.maximum, sample);
    if (sample != 0) {
      ++stats.nonZeroCount;
    }

    const int64_t signedSample = sample;
    const uint64_t magnitude = signedSample < 0
                                   ? static_cast<uint64_t>(-signedSample)
                                   : static_cast<uint64_t>(signedSample);
    stats.absoluteSum += magnitude;
  }

  return stats;
}

Ics43434Mic microphone;

}  //  OmiPetAudio 命名空间 / OmiPetAudio namespace
