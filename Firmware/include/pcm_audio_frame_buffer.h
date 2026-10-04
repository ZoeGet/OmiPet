#pragma once

#include <Arduino.h>

#include <cstddef>
#include <cstdint>

#include "ics43434_mic.h"

namespace OmiPetAudio {

//  AFE 输入音频帧配置 / AFE input audio frame configuration
constexpr size_t kPcmAudioFrameSamples = 160;  //  每个 PCM 音频帧的采样点数 / Samples in one PCM audio frame
constexpr size_t kPcmAudioFrameQueueCapacity = 8;  //  PCM 音频帧队列容量 / PCM audio-frame queue capacity

//  固定长度 PCM 音频帧缓冲 / Fixed-size PCM audio frame buffer
class PcmAudioFrameBuffer {
 public:
  //  清空音频帧队列 / Reset the audio frame queue
  void reset();

  //  将交错 I²S 数据转换并写入 PCM 队列 / Convert interleaved I²S data into the PCM queue
  size_t pushInterleavedWords(const int32_t* words, size_t wordCount,
                              MicChannel channel);

  //  读取一帧 16-bit 单声道 PCM / Read one frame of 16-bit mono PCM
  bool popFrame(int16_t* frame, size_t sampleCount);

  //  获取当前排队帧数 / Get the number of queued frames
  size_t queuedFrames() const;

  //  获取因队列满而丢弃的帧数 / Get the number of frames dropped because the queue was full
  uint32_t droppedFrames() const;

 private:
  //  追加一个 PCM 样本，样本满一帧后入队 / Append one PCM sample and enqueue a complete frame
  void appendSample(int16_t sample);

  int16_t pendingSamples_[kPcmAudioFrameSamples] = {};  //  当前正在拼接的 PCM 样本 / PCM samples currently being assembled
  size_t pendingSampleCount_ = 0;  //  当前待拼接样本数量 / Number of pending samples being assembled
  int16_t frameQueue_[kPcmAudioFrameQueueCapacity][kPcmAudioFrameSamples] = {};  //  已完成 PCM 帧存储区 / Storage for completed PCM frames
  size_t queueReadIndex_ = 0;  //  下一帧读取位置 / Index of the next frame to read
  size_t queueWriteIndex_ = 0;  //  下一帧写入位置 / Index of the next frame to write
  size_t queuedFrameCount_ = 0;  //  当前排队帧数量 / Current number of queued frames
  uint32_t droppedFrameCount_ = 0;  //  因队列满而丢弃的帧数量 / Number of frames dropped because the queue was full
};

//  将左对齐 24-bit I²S 样本转换为 16-bit PCM / Convert left-aligned 24-bit I²S samples to 16-bit PCM
int16_t convertI2sSampleToPcm16(int32_t rawSample);

}  //  OmiPetAudio 命名空间 / OmiPetAudio namespace
