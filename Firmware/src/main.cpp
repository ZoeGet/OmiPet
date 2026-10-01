#include <Arduino.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include "aht20_sensor.h"
#include "buzzer.h"
#include "NV3007_Display.h"
#include "led_strip.h"
#include "omi_pet_ui.h"
#include "wifi_manager.h"
#include "ics43434_mic.h"
#include "pcm_audio_frame_buffer.h"
#include "voice_controller.h"

namespace {

constexpr size_t kMicDiagnosticFrameCount = 800;
constexpr size_t kMicDiagnosticWordCount = kMicDiagnosticFrameCount * 2;
constexpr uint32_t kMicWindowIntervalMs = 50;
constexpr uint32_t kMicLogIntervalMs = 1000;
constexpr uint32_t kMinimumSpeechStartRms = 10000;
constexpr uint32_t kMinimumSpeechHoldRms = 5000;
constexpr uint8_t kSpeechStartWindowCount = 3;
constexpr uint8_t kSpeechEndWindowCount = 8;
int32_t gMicDiagnosticWords[kMicDiagnosticWordCount] = {};
uint32_t gLastMicWindowMs = 0;
uint32_t gLastMicLogMs = 0;
uint32_t gLastSystemHeartbeatMs = 0;
uint32_t gMicNoiseFloorRms = 0;
bool gMicNoiseFloorInitialized = false;
uint8_t gSpeechStartWindows = 0;
uint8_t gSpeechQuietWindows = 0;
bool gSpeechActive = false;
uint32_t gLastMicThreshold = 0;
bool gLastSpeechCandidate = false;

struct MicLevelStats {
  uint32_t averageAbsolute = 0;
  uint32_t rms = 0;
  uint32_t peak = 0;
  size_t frameCount = 0;
};

MicLevelStats gLastMicStats;
OmiPetAudio::PcmAudioFrameBuffer gWakeWordAudioBuffer;
int16_t gWakeWordPcmFrame[OmiPetAudio::kWakeWordFrameSamples] = {};
uint32_t gWakeWordFrameCount = 0;
uint32_t gLastAudioFrameLogMs = 0;

//  计算选定声道的音量统计 / Calculate level statistics for the selected channel
MicLevelStats analyzeMicrophoneLevel(const int32_t* words, size_t wordCount,
                                     OmiPetAudio::MicChannel channel) {
  MicLevelStats stats;
  const size_t selectedSlot = static_cast<size_t>(channel);
  uint64_t absoluteSum = 0;
  uint64_t squareSum = 0;

  const size_t frameCount = wordCount / 2;
  for (size_t frame = 0; frame < frameCount; ++frame) {
    const int32_t rawSample = words[frame * 2 + selectedSlot];
    const int32_t sample = rawSample >> 8;
    const int64_t signedSample = sample;
    const uint64_t magnitude = signedSample < 0
                                   ? static_cast<uint64_t>(-signedSample)
                                   : static_cast<uint64_t>(signedSample);
    absoluteSum += magnitude;
    squareSum += magnitude * magnitude;
    stats.peak = std::max(stats.peak, static_cast<uint32_t>(magnitude));
    ++stats.frameCount;
  }

  if (stats.frameCount == 0U) {
    return stats;
  }

  stats.averageAbsolute =
      static_cast<uint32_t>(absoluteSum / stats.frameCount);
  stats.rms = static_cast<uint32_t>(
      std::sqrt(static_cast<double>(squareSum) / stats.frameCount));
  return stats;
}

//  更新自适应噪声底并检测语音 / Update the adaptive noise floor and detect speech
bool detectSpeech(uint32_t rms, uint32_t& threshold) {
  if (!gMicNoiseFloorInitialized) {
    gMicNoiseFloorRms = rms;
    gMicNoiseFloorInitialized = true;
  }

  const uint32_t thresholdMultiplier = gSpeechActive ? 2U : 3U;
  threshold = std::max(gMicNoiseFloorRms * thresholdMultiplier,
                       gMicNoiseFloorRms + 1000U);
  const uint32_t minimumRms =
      gSpeechActive ? kMinimumSpeechHoldRms : kMinimumSpeechStartRms;
  const bool speechDetected = rms > threshold && rms >= minimumRms;
  if (!speechDetected) {
    gMicNoiseFloorRms = (gMicNoiseFloorRms * 15U + rms) / 16U;
  }
  return speechDetected;
}

//  更新连续语音状态并应用迟滞 / Update continuous speech state with hysteresis
bool updateSpeechState(bool speechCandidate) {
  if (speechCandidate) {
    gSpeechQuietWindows = 0;
    if (!gSpeechActive) {
      if (gSpeechStartWindows < 255U) {
        ++gSpeechStartWindows;
      }
      if (gSpeechStartWindows >= kSpeechStartWindowCount) {
        gSpeechActive = true;
        gSpeechStartWindows = 0;
      }
    }
  } else {
    gSpeechStartWindows = 0;
    if (gSpeechActive) {
      if (gSpeechQuietWindows < 255U) {
        ++gSpeechQuietWindows;
      }
      if (gSpeechQuietWindows >= kSpeechEndWindowCount) {
        gSpeechActive = false;
        gSpeechQuietWindows = 0;
      }
    }
  }
  return gSpeechActive;
}

//  每 50 ms 处理音频窗口，每秒打印一次状态 / Process an audio window every 50 ms and print status once per second
void updateMicrophoneDiagnostic() {
  const uint32_t nowMs = millis();
  if (nowMs - gLastMicWindowMs < kMicWindowIntervalMs) {
    return;
  }
  gLastMicWindowMs = nowMs;

  if (!OmiPetAudio::microphone.initialized()) {
    if (nowMs - gLastMicLogMs >= kMicLogIntervalMs) {
      gLastMicLogMs = nowMs;
      Serial.println("[MIC] unavailable");
    }
    return;
  }

  const size_t wordCount = OmiPetAudio::microphone.readRawWords(
      gMicDiagnosticWords, kMicDiagnosticWordCount);
  if (wordCount == 0U) {
    if (nowMs - gLastMicLogMs >= kMicLogIntervalMs) {
      gLastMicLogMs = nowMs;
      Serial.println("[MIC] no samples");
    }
    return;
  }

  gWakeWordAudioBuffer.pushInterleavedWords(
      gMicDiagnosticWords, wordCount, OmiPetAudio::microphone.channel());
  gLastMicStats = analyzeMicrophoneLevel(
      gMicDiagnosticWords, wordCount, OmiPetAudio::microphone.channel());
  gLastSpeechCandidate =
      detectSpeech(gLastMicStats.rms, gLastMicThreshold);
  const bool speechWasActive = gSpeechActive;
  updateSpeechState(gLastSpeechCandidate);
  if (speechWasActive != gSpeechActive) {
    Serial.printf("[VAD] event=%s rms=%lu threshold=%lu\n",
                  gSpeechActive ? "start" : "stop",
                  static_cast<unsigned long>(gLastMicStats.rms),
                  static_cast<unsigned long>(gLastMicThreshold));
  }
  if (nowMs - gLastMicLogMs < kMicLogIntervalMs) {
    return;
  }
  gLastMicLogMs = nowMs;
  Serial.printf(
      "[MIC] frames=%u avg_abs=%lu rms=%lu peak=%lu noise=%lu threshold=%lu candidate=%s speech=%s start=%u quiet=%u\n",
      static_cast<unsigned>(gLastMicStats.frameCount),
      static_cast<unsigned long>(gLastMicStats.averageAbsolute),
      static_cast<unsigned long>(gLastMicStats.rms),
      static_cast<unsigned long>(gLastMicStats.peak),
      static_cast<unsigned long>(gMicNoiseFloorRms),
      static_cast<unsigned long>(gLastMicThreshold),
      gLastSpeechCandidate ? "yes" : "no",
      gSpeechActive ? "yes" : "no",
      static_cast<unsigned>(gSpeechStartWindows),
      static_cast<unsigned>(gSpeechQuietWindows));
}

//  消费固定长度 PCM 音频帧并输出缓冲诊断 / Consume fixed-size PCM frames and print buffer diagnostics
void updateWakeWordAudioFrames() {
  size_t processedFrameCount = 0;
  while (gWakeWordAudioBuffer.popFrame(
      gWakeWordPcmFrame, OmiPetAudio::kWakeWordFrameSamples)) {
    ++processedFrameCount;
  }
  gWakeWordFrameCount += static_cast<uint32_t>(processedFrameCount);

  const uint32_t nowMs = millis();
  if (nowMs - gLastAudioFrameLogMs < kMicLogIntervalMs) {
    return;
  }
  gLastAudioFrameLogMs = nowMs;
  Serial.printf(
      "[AUDIO] pcm16_frames=%lu samples_per_frame=%u rate=%lu channel=right queued=%u dropped=%lu\n",
      static_cast<unsigned long>(gWakeWordFrameCount),
      static_cast<unsigned>(OmiPetAudio::kWakeWordFrameSamples),
      static_cast<unsigned long>(OmiPetAudio::kMicDefaultSampleRateHz),
      static_cast<unsigned>(gWakeWordAudioBuffer.queuedFrames()),
      static_cast<unsigned long>(gWakeWordAudioBuffer.droppedFrames()));
}
//  处理临时语音唤醒测试命令 / Process the temporary voice wake test command
void updateVoiceDebugInput() {
  while (Serial.available() > 0) {
    const int input = Serial.read();
    if (input == 'w') {
      Serial.println("[VOICE] debug wake command");
      OmiPetVoice::voice.notifyWakeWordDetected();
    }
  }
}
//  持续输出系统心跳 / Print a persistent system heartbeat
void updateSystemHeartbeat() {
  if (millis() - gLastSystemHeartbeatMs < 2000U) {
    return;
  }
  gLastSystemHeartbeatMs = millis();
  Serial.printf("[SYS] alive ms=%lu mic=%s wifi=%s\n",
                static_cast<unsigned long>(millis()),
                OmiPetAudio::microphone.initialized() ? "ok" : "off",
                OmiPetNetwork::wifi.connected() ? "connected" : "offline");
}

}  //  匿名命名空间 / Anonymous namespace

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println("[BOOT] setup entered");
  Serial.flush();

  //  初始化灯带并保持低亮度白光 / Initialize the LED strip and keep low-brightness white light
  OmiPetLed::strip.begin(16);
  OmiPetLed::strip.fill(255, 255, 255);

  //  初始化无源蜂鸣器，但不自动播放声音 / Initialize the passive buzzer without playing sound automatically
  OmiPetBuzzer::buzzer.begin();
  OmiPetVoice::voice.begin();

  //  初始化 LCD 并打开背光 / Initialize the LCD and enable the backlight
  OmiPetDisplay::lcd.begin(8000000UL);
  OmiPetDisplay::lcd.setBacklight(true);

  //  初始化 AHT20 并尝试获取首个测量值 / Initialize AHT20 and try to obtain the first measurement
  const bool sensorReady = OmiPetSensor::aht20.begin();
  if (sensorReady) {
    OmiPetSensor::aht20.readMeasurement();
  }
  const OmiPetSensor::Aht20Measurement& initialReading =
      OmiPetSensor::aht20.measurement();
  OmiPetUi::setEnvironment(initialReading.temperatureC,
                           initialReading.humidityPercent,
                           initialReading.valid && !initialReading.stale);
  //  先绘制界面，再执行可能等待 Wi-Fi 的连接流程 / Draw the UI before starting the potentially waiting Wi-Fi connection flow
  OmiPetUi::setNetworkStatus(false, false);
  OmiPetUi::begin();

  //  初始化 Wi-Fi；无已保存配置时开启网页配网热点 / Initialize Wi-Fi; start the web portal when no saved configuration exists
  OmiPetNetwork::wifi.begin();
  OmiPetUi::setNetworkStatus(OmiPetNetwork::wifi.connected(),
                             OmiPetNetwork::wifi.provisioning());
  //  初始化麦克风采集诊断 / Initialize the microphone capture diagnostic
  //  当前硬件使用右声道槽 / The current hardware uses the right I2S slot
  const bool microphoneReady = OmiPetAudio::microphone.begin(
      OmiPetAudio::kMicDefaultSampleRateHz, OmiPetAudio::MicChannel::Right);
  Serial.printf("[MIC] init=%s rate=%lu sck=%u ws=%u sd=%u\n",
                microphoneReady ? "ok" : "failed",
                static_cast<unsigned long>(OmiPetAudio::microphone.sampleRateHz()),
                static_cast<unsigned>(OmiPetAudio::kMicSckPin),
                static_cast<unsigned>(OmiPetAudio::kMicWsPin),
                static_cast<unsigned>(OmiPetAudio::kMicSdPin));
}

void loop() {
  //  处理 WiFiManager 网页配网和连接状态 / Process WiFiManager provisioning and connection state
  OmiPetNetwork::wifi.update();
  updateSystemHeartbeat();
  updateMicrophoneDiagnostic();
  updateWakeWordAudioFrames();
  updateVoiceDebugInput();
  OmiPetVoice::voice.update(gSpeechActive);
  OmiPetUi::setNetworkStatus(OmiPetNetwork::wifi.connected(),
                             OmiPetNetwork::wifi.provisioning());

  //  每 5 秒读取一次温湿度，降低传感器自热和总线占用 / Read every 5 seconds to reduce sensor self-heating and bus usage
  static uint32_t lastSensorReadMs = millis();
  if (millis() - lastSensorReadMs >= 5000U) {
    lastSensorReadMs = millis();
    OmiPetSensor::aht20.readMeasurement();
    const OmiPetSensor::Aht20Measurement& reading =
        OmiPetSensor::aht20.measurement();
    OmiPetUi::setEnvironment(reading.temperatureC, reading.humidityPercent,
                             reading.valid && !reading.stale);
  }
  //  维护蜂鸣器非阻塞播放状态 / Maintain the buzzer's non-blocking playback state
  OmiPetBuzzer::buzzer.update();
  OmiPetUi::update();
}
