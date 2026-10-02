#include "wake_word_detector.h"

#include "voice_controller.h"

extern "C" {
#include "esp_wn_iface.h"
#include "esp_wn_models.h"
#include "model_path.h"
}

//  ESP-SR WakeNet 后端适配实现 / ESP-SR WakeNet backend adapter implementation
namespace OmiPetAudio {

namespace {

constexpr char kModelPartitionLabel[] = "model";
//  当前仓库实际可用的模型仍然识别 Hi ESP / The currently available model still recognizes Hi ESP
constexpr char kWakeNetModelName[] = "wn9_hiesp";

}  //  匿名命名空间 / Anonymous namespace

bool WakeWordDetector::begin(uint32_t sampleRateHz, size_t frameSamples) {
  sampleRateHz_ = sampleRateHz;
  frameSamples_ = frameSamples;
  processedFrameCount_ = 0;
  state_ = WakeWordDetectorState::Unavailable;
  modelName_ = "not-configured";
  models_ = nullptr;
  wakeNet_ = nullptr;
  wakeNetModel_ = nullptr;

  if (sampleRateHz_ != 16000U || frameSamples_ == 0U) {
    return false;
  }

  Serial.printf("[WAKE] requested=%s active-model-word=Hi ESP\n",
                OmiPetVoice::kWakeWordPhrase);

  srmodel_list_t* models = esp_srmodel_init(kModelPartitionLabel);
  if (models == nullptr) {
    Serial.println("[WAKE] model partition unavailable");
    return false;
  }
  models_ = models;

  if (esp_srmodel_exists(models, const_cast<char*>(kWakeNetModelName)) < 0) {
    Serial.printf("[WAKE] model missing name=%s\n", kWakeNetModelName);
    return false;
  }

  const esp_wn_iface_t* wakeNet =
      esp_wn_handle_from_name(kWakeNetModelName);
  if (wakeNet == nullptr || wakeNet->create == nullptr) {
    Serial.println("[WAKE] WakeNet interface unavailable");
    return false;
  }

  model_iface_data_t* wakeNetModel =
      wakeNet->create(kWakeNetModelName, DET_MODE_90);
  if (wakeNetModel == nullptr) {
    Serial.println("[WAKE] WakeNet model create failed");
    return false;
  }

  const int modelRate = wakeNet->get_samp_rate(wakeNetModel);
  const int modelFrameSamples = wakeNet->get_samp_chunksize(wakeNetModel);
  if (modelRate != static_cast<int>(sampleRateHz_) ||
      modelFrameSamples != static_cast<int>(frameSamples_)) {
    Serial.printf("[WAKE] input mismatch model_rate=%d model_frames=%d\n",
                  modelRate, modelFrameSamples);
    wakeNet->destroy(wakeNetModel);
    return false;
  }

  wakeNet_ = const_cast<esp_wn_iface_t*>(wakeNet);
  wakeNetModel_ = wakeNetModel;
  modelName_ = kWakeNetModelName;
  state_ = WakeWordDetectorState::Ready;
  Serial.printf("[WAKE] model=%s rate=%lu frames=%u\n",
                modelName_, static_cast<unsigned long>(sampleRateHz_),
                static_cast<unsigned>(frameSamples_));
  return true;
}

//  将一帧 PCM 音频提交给 WakeNet / Submit one PCM frame to WakeNet
bool WakeWordDetector::processFrame(const int16_t* samples, size_t sampleCount) {
  if (samples == nullptr || sampleCount != frameSamples_ || !available()) {
    return false;
  }

  ++processedFrameCount_;
  const auto* wakeNet = static_cast<const esp_wn_iface_t*>(wakeNet_);
  auto* wakeNetModel = static_cast<model_iface_data_t*>(wakeNetModel_);
  return wakeNet != nullptr && wakeNet->detect != nullptr &&
         wakeNet->detect(wakeNetModel, const_cast<int16_t*>(samples)) ==
             WAKENET_DETECTED;
}

bool WakeWordDetector::available() const {
  return state_ == WakeWordDetectorState::Ready && wakeNet_ != nullptr &&
         wakeNetModel_ != nullptr;
}

const char* WakeWordDetector::backendName() const {
  return "esp-sr-wakenet";
}

const char* WakeWordDetector::modelName() const {
  return modelName_;
}

WakeWordDetectorState WakeWordDetector::state() const {
  return state_;
}

uint32_t WakeWordDetector::processedFrameCount() const {
  return processedFrameCount_;
}

WakeWordDetector wakeWordDetector;

}  //  OmiPetAudio 命名空间 / OmiPetAudio namespace
