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

//  OmiPet 固件主循环和模块编排 / OmiPet firmware entry point and module orchestration
#include "ics43434_mic.h"
#include "pcm_audio_frame_buffer.h"
#include "multinet_command_recognizer.h"
#include "voice_controller.h"

namespace {

//  音频链路为 I2S、PCM 队列、AFE、VAD 和 MultiNet 的串联流程 / The audio path chains I2S, PCM queue, AFE, VAD, and MultiNet
//  麦克风诊断窗口和周期参数 / Microphone diagnostic window and periodic timing parameters
constexpr size_t kMicDiagnosticFrameCount = 160;
constexpr size_t kMicDiagnosticWordCount = kMicDiagnosticFrameCount * 2;
constexpr uint32_t kMicWindowIntervalMs = 10;
constexpr uint32_t kMicLogIntervalMs = 1000;
//  语音结束后保留一段 MultiNet 检测尾窗，避免漏掉词尾 / Keep a MultiNet detection hangover after speech to avoid missing phrase endings
constexpr uint32_t kSpeechDetectionHangoverMs = 1200;
//  限制单次主循环处理的音频帧数，避免识别任务长期占满 CPU / Limit frames processed per loop so recognition cannot monopolize the CPU
constexpr size_t kMaxCommandFramesPerLoop = 4;
constexpr uint32_t kMinimumSpeechStartRms = 10000;
constexpr uint32_t kMinimumSpeechHoldRms = 5000;
constexpr uint8_t kSpeechStartWindowCount = 3;
constexpr uint8_t kSpeechEndWindowCount = 8;

//  麦克风采样、噪声底和 VAD 状态 / Microphone samples, noise floor, and VAD state
int32_t gMicDiagnosticWords[kMicDiagnosticWordCount] = {};
uint32_t gLastMicWindowMs = 0;
uint32_t gLastMicLogMs = 0;
uint32_t gLastSystemHeartbeatMs = 0;
uint32_t gMicNoiseFloorRms = 0;
bool gMicNoiseFloorInitialized = false;
uint8_t gSpeechStartWindows = 0;
uint8_t gSpeechQuietWindows = 0;
bool gSpeechActive = false;
//  记录最近一次 VAD 语音活动，用于维持检测尾窗 / Track the latest VAD activity to maintain the detection hangover
uint32_t gLastSpeechActivityMs = 0;
uint32_t gLastMicThreshold = 0;
bool gLastSpeechCandidate = false;

struct MicLevelStats {
  uint32_t averageAbsolute = 0;
  uint32_t rms = 0;
  uint32_t peak = 0;
  size_t frameCount = 0;
};

MicLevelStats gLastMicStats;

//  命令音频队列和识别时间戳 / Command audio queue and recognition timestamps
OmiPetAudio::PcmAudioFrameBuffer gCommandAudioBuffer;
int16_t gCommandPcmFrame[OmiPetAudio::kPcmAudioFrameSamples] = {};
uint32_t gCommandFrameCount = 0;
uint32_t gLastAudioFrameLogMs = 0;
uint32_t gSpeechStartAtMs = 0;
uint32_t gSpeechStopAtMs = 0;
uint32_t gWakeAcceptedAtMs = 0;
bool gSpeechTimingValid = false;

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

//  根据动态噪声底估算语音候选状态 / Estimate speech candidacy from the adaptive noise floor
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

//  读取麦克风窗口、更新 VAD 状态，并把 PCM 帧送入识别队列 / Read microphone windows, update VAD, and queue PCM frames
//  每 10 ms 处理音频窗口，每秒打印一次诊断状态 / Process an audio window every 10 ms and print diagnostics once per second
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

  gLastMicStats = analyzeMicrophoneLevel(
      gMicDiagnosticWords, wordCount, OmiPetAudio::microphone.channel());
  if (OmiPetAudio::multiNetCommandRecognizer.available()) {
    //  无论是否允许识别，都先把音频送入 AFE 处理链 / Always feed audio into the AFE pipeline, even when detection is gated off
    gCommandAudioBuffer.pushInterleavedWords(
        gMicDiagnosticWords, wordCount, OmiPetAudio::microphone.channel());
  }
  const bool speechWasActive = gSpeechActive;
  if (OmiPetVoice::voice.speechInputSuppressed()) {
    gLastSpeechCandidate = false;
    gSpeechActive = false;
    gSpeechStartWindows = 0;
    gSpeechQuietWindows = 0;
  } else {
    gLastSpeechCandidate =
        detectSpeech(gLastMicStats.rms, gLastMicThreshold);
    if (gLastSpeechCandidate) {
      gLastSpeechActivityMs = nowMs;
    }
    updateSpeechState(gLastSpeechCandidate);
  }
  if (speechWasActive != gSpeechActive) {
    Serial.printf("[VAD] event=%s rms=%lu threshold=%lu\n",
                  gSpeechActive ? "start" : "stop",
                  static_cast<unsigned long>(gLastMicStats.rms),
                  static_cast<unsigned long>(gLastMicThreshold));
    if (gSpeechActive) {
      gSpeechStartAtMs = nowMs;
      gSpeechTimingValid = true;
      Serial.printf("[TIMING] speech_start ms=%lu\n",
                    static_cast<unsigned long>(gSpeechStartAtMs));
    } else {
      gSpeechStopAtMs = nowMs;
      Serial.printf("[TIMING] speech_stop ms=%lu duration_ms=%lu\n",
                    static_cast<unsigned long>(gSpeechStopAtMs),
                    gSpeechTimingValid
                        ? static_cast<unsigned long>(gSpeechStopAtMs -
                                                     gSpeechStartAtMs)
                        : 0UL);
    }
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

//  将连续采集的 PCM 帧送入离线命令识别器 / Submit continuously captured PCM frames to the offline command recognizer
//  AFE 始终接收音频，allowDetection 只控制 MultiNet 是否实际判定 / AFE always receives audio; allowDetection only gates MultiNet decisions
void updateCommandAudioFrames() {
  size_t processedFrameCount = 0;
  while (gCommandAudioBuffer.popFrame(
             gCommandPcmFrame, OmiPetAudio::kPcmAudioFrameSamples) &&
         processedFrameCount < kMaxCommandFramesPerLoop) {
    ++processedFrameCount;
    const uint32_t nowMs = millis();
    //  AFE 持续运行，但只有语音活动或尾窗内才调用 MultiNet 推理 / Keep AFE running continuously, but invoke MultiNet only during speech or hangover
    const bool speechDetectionWindowOpen =
        gSpeechActive ||
        (gLastSpeechActivityMs != 0U &&
         nowMs - gLastSpeechActivityMs <= kSpeechDetectionHangoverMs);
    const bool allowDetection =
        speechDetectionWindowOpen &&
        !OmiPetVoice::voice.speechInputSuppressed();
    const uint32_t detectStartedAtMs = millis();
    const int commandId = OmiPetAudio::multiNetCommandRecognizer.processFrame(
        gCommandPcmFrame, OmiPetAudio::kPcmAudioFrameSamples,
        allowDetection);
    const uint32_t detectFinishedAtMs = millis();
    //  自定义词 ID=1 作为待机入口，识别后清空上一段模型上下文 / Use custom phrase ID 1 as the idle entry and clear the previous model context
    if (commandId == OmiPetAudio::kWakePhraseCommandId &&
        OmiPetVoice::voice.state() == OmiPetVoice::VoiceState::Idle) {
      const uint32_t speechAgeMs =
          gSpeechTimingValid ? detectFinishedAtMs - gSpeechStartAtMs : 0U;
      const uint32_t speechStopAgeMs =
          gSpeechTimingValid && gSpeechStopAtMs >= gSpeechStartAtMs
              ? detectFinishedAtMs - gSpeechStopAtMs
              : 0U;
      Serial.printf(
          "[TIMING] wake_result ms=%lu infer_ms=%lu since_speech_start_ms=%lu "
          "since_speech_stop_ms=%lu\n",
          static_cast<unsigned long>(detectFinishedAtMs),
          static_cast<unsigned long>(detectFinishedAtMs - detectStartedAtMs),
          static_cast<unsigned long>(speechAgeMs),
          static_cast<unsigned long>(speechStopAgeMs));
      OmiPetAudio::multiNetCommandRecognizer.reset();
      gCommandAudioBuffer.reset();
      Serial.println("[VOICE] continuous wake phrase detected");
      if (OmiPetVoice::voice.notifyWakeWordDetected()) {
        gWakeAcceptedAtMs = millis();
        Serial.printf("[TIMING] wake_accepted ms=%lu\n",
                      static_cast<unsigned long>(gWakeAcceptedAtMs));
      }
      continue;
    }
    //  唤醒后只接受有限的亮度命令 ID，执行成功后回到持续监听 / After wake-up accept only brightness command IDs, then return to continuous listening
    if ((commandId == OmiPetAudio::kIncreaseBrightnessCommandId ||
         commandId == OmiPetAudio::kDecreaseBrightnessCommandId) &&
        OmiPetVoice::voice.listeningForCommand()) {
      const uint32_t speechAgeMs =
          gSpeechTimingValid ? detectFinishedAtMs - gSpeechStartAtMs : 0U;
      const uint32_t wakeAgeMs = gWakeAcceptedAtMs != 0U
                                      ? detectFinishedAtMs - gWakeAcceptedAtMs
                                      : 0U;
      Serial.printf(
          "[TIMING] command_result ms=%lu infer_ms=%lu "
          "since_speech_start_ms=%lu since_wake_accepted_ms=%lu\n",
          static_cast<unsigned long>(detectFinishedAtMs),
          static_cast<unsigned long>(detectFinishedAtMs - detectStartedAtMs),
          static_cast<unsigned long>(speechAgeMs),
          static_cast<unsigned long>(wakeAgeMs));
      const int currentBrightness = OmiPetLed::strip.brightness();
      const int brightnessStep =
          commandId == OmiPetAudio::kIncreaseBrightnessCommandId ? 16 : -16;
      const int updatedBrightness = std::max(
          0, std::min(currentBrightness + brightnessStep, 255));
      OmiPetLed::strip.setBrightness(static_cast<uint8_t>(updatedBrightness));
      OmiPetLed::strip.show();
      Serial.printf("[LED] brightness=%d command=%s\n", updatedBrightness,
                    commandId == OmiPetAudio::kIncreaseBrightnessCommandId
                        ? "increase"
                        : "decrease");
      Serial.printf("[TIMING] command_executed ms=%lu detect_to_led_ms=%lu\n",
                    static_cast<unsigned long>(millis()),
                    static_cast<unsigned long>(millis() - detectFinishedAtMs));
      OmiPetAudio::multiNetCommandRecognizer.reset();
      OmiPetVoice::voice.notifyCommandCompleted();
    }
    //  每帧主动让出 CPU，确保系统空闲任务和看门狗获得调度 / Yield after each frame so idle tasks and the watchdog can run
    yield();
  }
  gCommandFrameCount += static_cast<uint32_t>(processedFrameCount);

  const uint32_t nowMs = millis();
  if (nowMs - gLastAudioFrameLogMs < kMicLogIntervalMs) {
    return;
  }
  gLastAudioFrameLogMs = nowMs;
  Serial.printf(
      "[AUDIO] command_frames=%lu samples_per_frame=%u rate=%lu channel=right queued=%u dropped=%lu\n",
      static_cast<unsigned long>(gCommandFrameCount),
      static_cast<unsigned>(OmiPetAudio::kPcmAudioFrameSamples),
      static_cast<unsigned long>(OmiPetAudio::kMicDefaultSampleRateHz),
      static_cast<unsigned>(gCommandAudioBuffer.queuedFrames()),
      static_cast<unsigned long>(gCommandAudioBuffer.droppedFrames()));
}
//  处理串口临时唤醒测试命令，不参与正常唤醒路径 / Process the temporary serial wake test command; it is not part of normal wake detection
void updateVoiceDebugInput() {
  while (Serial.available() > 0) {
    const int input = Serial.read();
    if (input == 'w') {
      if (OmiPetVoice::voice.state() == OmiPetVoice::VoiceState::Idle) {
        gCommandAudioBuffer.reset();
        OmiPetAudio::multiNetCommandRecognizer.reset();
        Serial.println("[VOICE] serial wake trigger (debug fallback; continuous recognition active)");
        OmiPetVoice::voice.notifyWakeWordDetected();
      }
    }
  }
}
//  持续输出系统心跳，帮助区分识别失败和系统停滞 / Print a persistent heartbeat to distinguish recognition failures from a stalled system
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

//  初始化所有硬件驱动和业务模块 / Initialize all hardware drivers and application modules
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

  const bool recognizerReady = OmiPetAudio::multiNetCommandRecognizer.begin(
      OmiPetAudio::kMicDefaultSampleRateHz, OmiPetAudio::kPcmAudioFrameSamples);
  Serial.printf("[ASR] init=%s frame=%u\n",
                recognizerReady ? "ready" : "unavailable",
                static_cast<unsigned>(OmiPetAudio::kPcmAudioFrameSamples));
}

//  按固定顺序执行非阻塞更新：网络、采集、识别、传感器和 UI / Run non-blocking updates in order: network, capture, recognition, sensors, and UI
void loop() {
  //  处理 WiFiManager 网页配网和连接状态 / Process WiFiManager provisioning and connection state
  OmiPetNetwork::wifi.update();
  updateSystemHeartbeat();
  updateMicrophoneDiagnostic();
  updateCommandAudioFrames();
  updateVoiceDebugInput();
  const bool wasListeningForCommand =
      OmiPetVoice::voice.listeningForCommand();
  OmiPetVoice::voice.update(gSpeechActive);
  if (wasListeningForCommand &&
      !OmiPetVoice::voice.listeningForCommand()) {
    OmiPetAudio::multiNetCommandRecognizer.reset();
    gCommandAudioBuffer.reset();
  }
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
