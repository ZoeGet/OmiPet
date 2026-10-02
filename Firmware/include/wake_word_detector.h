#pragma once

#include <Arduino.h>

#include <cstddef>
#include <cstdint>

namespace OmiPetAudio {

//  唤醒词后端状态 / Wake-word backend state
enum class WakeWordDetectorState : uint8_t {
  Unavailable,
  Ready,
};

//  唤醒词检测后端适配层 / Wake-word detector backend adapter
class WakeWordDetector {
 public:
  //  初始化唤醒词输入契约 / Initialize the wake-word input contract
  bool begin(uint32_t sampleRateHz, size_t frameSamples);

  //  提交一帧 16-bit 单声道 PCM / Submit one 16-bit mono PCM frame
  bool processFrame(const int16_t* samples, size_t sampleCount);

  //  查询当前后端是否可执行推理 / Check whether the backend can run inference
  bool available() const;

  //  获取后端名称 / Get the backend name
  const char* backendName() const;

  //  获取当前模型名称 / Get the active model name
  const char* modelName() const;

  //  获取当前后端状态 / Get the backend state
  WakeWordDetectorState state() const;

  //  获取已提交帧数 / Get the number of submitted frames
  uint32_t processedFrameCount() const;

 private:
  WakeWordDetectorState state_ = WakeWordDetectorState::Unavailable;
  uint32_t sampleRateHz_ = 0;
  size_t frameSamples_ = 0;
  uint32_t processedFrameCount_ = 0;
  const char* modelName_ = "not-configured";
  void* models_ = nullptr;
  void* wakeNet_ = nullptr;
  void* wakeNetModel_ = nullptr;
};

extern WakeWordDetector wakeWordDetector;

}  //  OmiPetAudio 命名空间 / OmiPetAudio namespace
