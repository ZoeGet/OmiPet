#include "wake_word_detector.h"

namespace OmiPetAudio {

bool WakeWordDetector::begin(uint32_t sampleRateHz, size_t frameSamples) {
  sampleRateHz_ = sampleRateHz;
  frameSamples_ = frameSamples;
  processedFrameCount_ = 0;
  state_ = WakeWordDetectorState::Unavailable;
  return sampleRateHz_ == 16000U && frameSamples_ == 480U;
}

bool WakeWordDetector::processFrame(const int16_t* samples, size_t sampleCount) {
  if (samples == nullptr || sampleCount != frameSamples_) {
    return false;
  }

  ++processedFrameCount_;
  return false;
}

bool WakeWordDetector::available() const {
  return state_ == WakeWordDetectorState::Ready;
}

const char* WakeWordDetector::backendName() const {
  return "none";
}

WakeWordDetectorState WakeWordDetector::state() const {
  return state_;
}

uint32_t WakeWordDetector::processedFrameCount() const {
  return processedFrameCount_;
}

WakeWordDetector wakeWordDetector;

}  //  OmiPetAudio 命名空间 / OmiPetAudio namespace
