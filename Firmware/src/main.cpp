#include <Arduino.h>
#include "aht20_sensor.h"
#include "buzzer.h"
#include "NV3007_Display.h"
#include "led_strip.h"
#include "omi_pet_ui.h"
#include "wifi_manager.h"
#include "ics43434_mic.h"

namespace {

constexpr size_t kMicDiagnosticWordCount = 64;
int32_t gMicDiagnosticWords[kMicDiagnosticWordCount] = {};
uint32_t gLastMicDiagnosticMs = 0;
uint32_t gLastSystemHeartbeatMs = 0;

//  每秒打印一次麦克风原始统计 / Print raw microphone statistics once per second
void updateMicrophoneDiagnostic() {
  if (millis() - gLastMicDiagnosticMs < 1000U) {
    return;
  }
  gLastMicDiagnosticMs = millis();

  if (!OmiPetAudio::microphone.initialized()) {
    Serial.println("[MIC] unavailable");
    return;
  }

  Serial.println("[MIC] read begin");
  Serial.flush();
  const size_t wordCount = OmiPetAudio::microphone.readRawWords(
      gMicDiagnosticWords, kMicDiagnosticWordCount);
  if (wordCount == 0U) {
    Serial.println("[MIC] no samples");
    return;
  }

  const OmiPetAudio::RawSampleStats stats =
      OmiPetAudio::analyzeRawSamples(gMicDiagnosticWords, wordCount);
  const uint64_t averageAbsolute = stats.absoluteSum / stats.sampleCount;
  Serial.printf("[MIC] words=%u min=%ld max=%ld nonzero=%u avg_abs=%llu\n",
                static_cast<unsigned>(stats.sampleCount),
                static_cast<long>(stats.minimum),
                static_cast<long>(stats.maximum),
                static_cast<unsigned>(stats.nonZeroCount),
                static_cast<unsigned long long>(averageAbsolute));
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
  const bool microphoneReady = OmiPetAudio::microphone.begin();
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
