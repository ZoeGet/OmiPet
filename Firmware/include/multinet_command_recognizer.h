#pragma once

#include <Arduino.h>

#include <cstddef>
#include <cstdint>

namespace OmiPetAudio {

//  连续识别实验使用的命令 ID / Command IDs used by the continuous recognition experiment
constexpr int kIncreaseBrightnessCommandId = 1;
constexpr int kDecreaseBrightnessCommandId = 2;
constexpr int kWakePhraseCommandId = 3;

class MultiNetCommandRecognizer {
 public:
  //  初始化 AFE、MultiNet 和当前项目的命令词表 / Initialize AFE, MultiNet, and the project's command phrase list
  bool begin(uint32_t sampleRateHz, size_t inputFrameSamples);
  //  始终推进 AFE，按语音活动状态选择是否执行 MultiNet 推理 / Always advance AFE and gate MultiNet inference by speech activity
  int processFrame(const int16_t* samples, size_t sampleCount,
                   bool allowDetection);
  //  清空 AFE 和 MultiNet 的上下文，开始新的语音片段 / Clear AFE and MultiNet state before a new speech segment
  void reset();
  //  查询识别器是否已完成初始化 / Check whether the recognizer is initialized
  bool available() const;
  //  获取 AFE 要求的输入帧采样数 / Get the input frame size required by AFE
  size_t frameSamples() const;

 private:
  void* modelList_ = nullptr;
  void* afeHandle_ = nullptr;
  void* afeData_ = nullptr;
  void* multiNet_ = nullptr;
  void* multiNetData_ = nullptr;
  size_t feedFrameSamples_ = 0;
  size_t fetchFrameSamples_ = 0;
  size_t modelFrameSamples_ = 0;
  size_t feedFramesPerFetch_ = 0;
  size_t feedSamplesSinceFetch_ = 0;
  bool ready_ = false;
};

extern MultiNetCommandRecognizer multiNetCommandRecognizer;

}  //  OmiPetAudio 命名空间 / OmiPetAudio namespace
