#include "pcm_audio_frame_buffer.h"

#include <algorithm>
#include <limits>

namespace OmiPetAudio {

void PcmAudioFrameBuffer::reset() {
  pendingSampleCount_ = 0;
  queueReadIndex_ = 0;
  queueWriteIndex_ = 0;
  queuedFrameCount_ = 0;
  droppedFrameCount_ = 0;
}

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

bool PcmAudioFrameBuffer::popFrame(int16_t* frame, size_t sampleCount) {
  if (frame == nullptr || sampleCount != kWakeWordFrameSamples ||
      queuedFrameCount_ == 0U) {
    return false;
  }

  std::copy_n(frameQueue_[queueReadIndex_], kWakeWordFrameSamples, frame);
  queueReadIndex_ = (queueReadIndex_ + 1U) % kWakeWordFrameQueueCapacity;
  --queuedFrameCount_;
  return true;
}

size_t PcmAudioFrameBuffer::queuedFrames() const {
  return queuedFrameCount_;
}

uint32_t PcmAudioFrameBuffer::droppedFrames() const {
  return droppedFrameCount_;
}

void PcmAudioFrameBuffer::appendSample(int16_t sample) {
  pendingSamples_[pendingSampleCount_++] = sample;
  if (pendingSampleCount_ < kWakeWordFrameSamples) {
    return;
  }

  if (queuedFrameCount_ >= kWakeWordFrameQueueCapacity) {
    ++droppedFrameCount_;
  } else {
    std::copy_n(pendingSamples_, kWakeWordFrameSamples,
                frameQueue_[queueWriteIndex_]);
    queueWriteIndex_ = (queueWriteIndex_ + 1U) % kWakeWordFrameQueueCapacity;
    ++queuedFrameCount_;
  }
  pendingSampleCount_ = 0;
}

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
