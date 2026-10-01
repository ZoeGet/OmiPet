#include <Arduino.h>
#include "aht20_sensor.h"
#include "buzzer.h"
#include "NV3007_Display.h"
#include "led_strip.h"
#include "omi_pet_ui.h"
#include "wifi_manager.h"

void setup() {
  // 初始化灯带并保持低亮度白光 / Initialize the LED strip and keep low-brightness white light
  OmiPetLed::strip.begin(16);
  OmiPetLed::strip.fill(255, 255, 255);

  // 初始化无源蜂鸣器，但不自动播放声音 / Initialize the passive buzzer without playing sound automatically
  OmiPetBuzzer::buzzer.begin();

  // 初始化 LCD 并打开背光 / Initialize the LCD and enable the backlight
  OmiPetDisplay::lcd.begin(8000000UL);
  OmiPetDisplay::lcd.setBacklight(true);

  // 初始化 Wi-Fi；无已保存配置时开启网页配网热点 / Initialize Wi-Fi; start the web portal when no saved configuration exists
  OmiPetNetwork::wifi.begin();

  // 初始化 AHT20 并尝试获取首个测量值 / Initialize AHT20 and try to obtain the first measurement
  const bool sensorReady = OmiPetSensor::aht20.begin();
  if (sensorReady) {
    OmiPetSensor::aht20.readMeasurement();
  }
  const OmiPetSensor::Aht20Measurement& initialReading =
      OmiPetSensor::aht20.measurement();
  OmiPetUi::setEnvironment(initialReading.temperatureC,
                           initialReading.humidityPercent,
                           initialReading.valid && !initialReading.stale);
  OmiPetUi::begin();
}

void loop() {
  // 处理 WiFiManager 网页配网和连接状态 / Process WiFiManager provisioning and connection state
  OmiPetNetwork::wifi.update();

  // 每 5 秒读取一次温湿度，降低传感器自热和总线占用 / Read every 5 seconds to reduce sensor self-heating and bus usage
  static uint32_t lastSensorReadMs = millis();
  if (millis() - lastSensorReadMs >= 5000U) {
    lastSensorReadMs = millis();
    OmiPetSensor::aht20.readMeasurement();
    const OmiPetSensor::Aht20Measurement& reading =
        OmiPetSensor::aht20.measurement();
    OmiPetUi::setEnvironment(reading.temperatureC, reading.humidityPercent,
                             reading.valid && !reading.stale);
  }
  // 维护蜂鸣器非阻塞播放状态 / Maintain the buzzer's non-blocking playback state
  OmiPetBuzzer::buzzer.update();
  OmiPetUi::update();
}
