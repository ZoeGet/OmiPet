#pragma once

#include <Arduino.h>
#include <driver/i2s.h>

namespace OmiPetAudio {

//  ICS-43434 硬件引脚映射 / ICS-43434 hardware mapping
constexpr uint8_t kMicSckPin = 5;
constexpr uint8_t kMicWsPin = 6;
constexpr uint8_t kMicSdPin = 7;
constexpr uint32_t kMicDefaultSampleRateHz = 16000;
constexpr uint8_t kMicBitsPerSample = 32;

//  当前硬件实测有效声道为右声道槽，原始采集保留双声道槽用于验证 / Current hardware testing confirms the right I2S slot; raw capture keeps both slots for validation
enum class MicChannel : uint8_t {
  Left = 0,
  Right = 1,
};

struct RawSampleStats {
  int32_t minimum = 0;
  int32_t maximum = 0;
  uint64_t absoluteSum = 0;
  uint32_t nonZeroCount = 0;
  size_t sampleCount = 0;
};

//  ICS-43434 I2S 采集薄封装 / Thin ICS-43434 I2S capture wrapper
class Ics43434Mic {
 public:
  Ics43434Mic() = default;

  //  使用 32-bit 槽启动 Philips I2S 接收 / Start Philips I2S RX with 32-bit slots
  bool begin(uint32_t sampleRateHz = kMicDefaultSampleRateHz,
             MicChannel channel = MicChannel::Right);

  //  停止 I2S 外设 / Stop the I2S peripheral
  void end();

  //  检查采集外设是否已初始化 / Check whether the capture peripheral is initialized
  bool initialized() const;

  //  返回已缓冲的 32-bit 原始数据字数 / Return buffered raw 32-bit words
  size_t availableRawWords();

  //  读取交错排列的左右声道原始槽数据 / Read interleaved raw left/right slot words
  size_t readRawWords(int32_t* buffer, size_t wordCount);

  //  只读取配置声道的数据 / Read only the configured slot from interleaved stereo frames
  size_t readSelectedFrames(int32_t* buffer, size_t frameCount);

  //  获取当前选择的麦克风声道槽 / Get the selected microphone slot
  MicChannel channel() const;

  //  获取当前采样率 / Get the configured sample rate
  uint32_t sampleRateHz() const;

 private:
  uint32_t sampleRateHz_ = kMicDefaultSampleRateHz;
  MicChannel channel_ = MicChannel::Right;
  bool initialized_ = false;
  i2s_port_t port_ = I2S_NUM_0;
};

//  在不假设位对齐方式的前提下分析原始样本 / Analyze raw samples without assuming their bit alignment
RawSampleStats analyzeRawSamples(const int32_t* samples, size_t sampleCount);

extern Ics43434Mic microphone;

}  //  OmiPetAudio 命名空间 / OmiPetAudio namespace
