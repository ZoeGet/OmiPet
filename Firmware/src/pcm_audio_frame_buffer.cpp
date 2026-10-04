#include "pcm_audio_frame_buffer.h"

//  固定长度 PCM 音频帧队列实现 / Fixed-size PCM audio frame queue implementation
#include <algorithm>
#include <limits>

namespace OmiPetAudio {

//  清空待组帧数据、已排队帧和丢帧计数 / Clear pending samples, queued frames, and drop counters
void PcmAudioFrameBuffer::reset() {
  pendingSampleCount_ = 0;
  queueReadIndex_ = 0;
  queueWriteIndex_ = 0;
  queuedFrameCount_ = 0;
  droppedFrameCount_ = 0;
}

//  把交错 I2S 双声道数据转换为单声道 PCM，并组成完整音频帧 / Convert interleaved I2S stereo data to mono PCM frames
size_t PcmAudioFrameBuffer::pushInterleavedWords(const int32_t* words,
                                                 size_t wordCount,
                                                 MicChannel channel) {
  if (words == nullptr || wordCount < 2U) {
    return 0;
  }

  const size_t selectedSlot = static_cast<size_t>(channel);
  const size_t frameCount = wordCount / 2U;
  for (size_t frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
    appendSample(convertI2sSampleToPcm16(words[frameIndex * 2U + selectedSlot]));
  }
  return frameCount;
}

//  从队列取出一帧完整 PCM 数据 / Pop one complete PCM frame from the queue
bool PcmAudioFrameBuffer::popFrame(int16_t* frame, size_t sampleCount) {
  if (frame == nullptr || sampleCount != kPcmAudioFrameSamples ||
      queuedFrameCount_ == 0U) {
    return false;
  }

  std::copy_n(frameQueue_[queueReadIndex_], kPcmAudioFrameSamples, frame);
  queueReadIndex_ = (queueReadIndex_ + 1U) % kPcmAudioFrameQueueCapacity;
  --queuedFrameCount_;
  return true;
}

//  返回当前等待处理的 PCM 帧数量 / Return the number of queued PCM frames
size_t PcmAudioFrameBuffer::queuedFrames() const {
  return queuedFrameCount_;
}

//  返回队列满时丢弃的帧总数 / Return frames dropped when the queue was full
uint32_t PcmAudioFrameBuffer::droppedFrames() const {
  return droppedFrameCount_;
}

//  将一个采样追加到当前写入帧 / Append one sample to the current write frame
void PcmAudioFrameBuffer::appendSample(int16_t sample) {
  pendingSamples_[pendingSampleCount_++] = sample;
  if (pendingSampleCount_ < kPcmAudioFrameSamples) {
    return;
  }

  if (queuedFrameCount_ >= kPcmAudioFrameQueueCapacity) {
    ++droppedFrameCount_;
  } else {
    std::copy_n(pendingSamples_, kPcmAudioFrameSamples,
                frameQueue_[queueWriteIndex_]);
    queueWriteIndex_ = (queueWriteIndex_ + 1U) % kPcmAudioFrameQueueCapacity;
    ++queuedFrameCount_;
  }
  pendingSampleCount_ = 0;
}

//  将 I2S 容器样本转换为 AFE PCM16 / Convert an I2S container sample to AFE PCM16
//  把 I2S 原始样本缩放并限制到 PCM16 范围 / Scale and clamp a raw I2S sample into the PCM16 range
int16_t convertI2sSampleToPcm16(int32_t rawSample) {
  const int32_t shiftedSample = rawSample >> 16;
  const int32_t minimumSample =
      static_cast<int32_t>(std::numeric_limits<int16_t>::min());
  const int32_t maximumSample =
      static_cast<int32_t>(std::numeric_limits<int16_t>::max());
  const int32_t clampedSample =
      std::max(minimumSample, std::min(shiftedSample, maximumSample));
  return static_cast<int16_t>(clampedSample);
}

}  //  OmiPetAudio 命名空间 / OmiPetAudio namespace
