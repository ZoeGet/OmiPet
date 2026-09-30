#include "aht20_sensor.h"

namespace OmiPetSensor {
namespace {

// AHT20 状态位和时序参数 / AHT20 status bits and timing parameters
constexpr uint8_t kStatusBusyMask = 0x80;
constexpr uint8_t kStatusCalibrationMask = 0x08;
constexpr uint32_t kPowerUpDelayMs = 100;
constexpr uint32_t kMeasurementStartDelayMs = 10;
constexpr uint32_t kMeasurementConversionDelayMs = 80;
constexpr uint32_t kMeasurementTimeoutMs = 120;
constexpr uint32_t kMeasurementPollIntervalMs = 5;
constexpr uint32_t kRecoveryFailureThreshold = 3;
constexpr uint32_t kRecoveryIntervalMs = 5000;

// AHT20 初始化、测量和软复位命令 / AHT20 initialization, measurement, and soft-reset commands
constexpr uint8_t kInitializeCommand[] = {0xBE, 0x08, 0x00};
constexpr uint8_t kMeasureCommand[] = {0xAC, 0x33, 0x00};
constexpr uint8_t kSoftResetCommand[] = {0xBA};

}  // 匿名命名空间 / Anonymous namespace

bool Aht20Sensor::begin(TwoWire& wire, uint8_t sda, uint8_t scl,
                        uint32_t frequency) {
  // 保存总线配置并清空上一轮状态 / Save bus settings and clear the previous state
  wire_ = &wire;
  sdaPin_ = sda;
  sclPin_ = scl;
  frequency_ = frequency;
  initialized_ = false;
  consecutiveFailures_ = 0;
  measurement_ = Aht20Measurement{};
  clearError();

  // 初始化 I2C 并设置总线频率 / Initialize I2C and set the bus frequency
  wire_->begin(sdaPin_, sclPin_);
  wire_->setClock(frequency_);
  // 等待传感器完成上电 / Wait for the sensor to complete power-up
  delay(kPowerUpDelayMs);

  // 先检测器件，再确认内部校准状态 / Probe the device, then verify internal calibration
  if (!probe() || !ensureCalibration()) {
    return false;
  }

  initialized_ = true;
  return true;
}

bool Aht20Sensor::readMeasurement() {
  if (!initialized_ || wire_ == nullptr) {
    return fail(Aht20Error::NotInitialized);
  }

  clearError();
  // 按手册要求，触发测量前预留命令间隔 / Reserve the command interval required by the datasheet
  delay(kMeasurementStartDelayMs);
  if (!sendCommand(kMeasureCommand, sizeof(kMeasureCommand))) {
    return false;
  }
  // 按手册要求等待测量转换完成 / Wait for the conversion time required by the datasheet
  delay(kMeasurementConversionDelayMs);
  if (!waitUntilReady(kMeasurementTimeoutMs)) {
    return false;
  }

  // 读取状态、湿度、温度和 CRC 共 7 个字节 / Read 7 bytes containing status, humidity, temperature, and CRC
  uint8_t frame[7] = {};
  if (!readFrame(frame, sizeof(frame))) {
    return false;
  }
  if (calculateCrc(frame, 6) != frame[6]) {
    return fail(Aht20Error::CrcMismatch);
  }

  // 从 20-bit 原始数据恢复湿度和温度 / Decode humidity and temperature from 20-bit raw values
  const uint32_t rawHumidity =
      ((static_cast<uint32_t>(frame[1]) << 12) |
       (static_cast<uint32_t>(frame[2]) << 4) | (frame[3] >> 4)) & 0xFFFFFU;
  const uint32_t rawTemperature =
      (((static_cast<uint32_t>(frame[3]) & 0x0FU) << 16) |
       (static_cast<uint32_t>(frame[4]) << 8) | frame[5]) & 0xFFFFFU;

  // 按 AHT20 公式换算为工程单位 / Convert to engineering units using the AHT20 formulas
  const float humidity =
      static_cast<float>(rawHumidity) * 100.0F / 1048576.0F;
  const float temperature =
      static_cast<float>(rawTemperature) * 200.0F / 1048576.0F - 50.0F;
  if (humidity < 0.0F || humidity > 100.0F || temperature < -40.0F ||
      temperature > 120.0F) {
    return fail(Aht20Error::OutOfRange);
  }

  // 只在 CRC 和范围均通过后发布新数据 / Publish new data only after CRC and range checks pass
  measurement_.temperatureC = temperature;
  measurement_.humidityPercent = humidity;
  measurement_.valid = true;
  measurement_.stale = false;
  measurement_.timestampMs = millis();
  consecutiveFailures_ = 0;
  return true;
}

const Aht20Measurement& Aht20Sensor::measurement() const {
  return measurement_;
}

Aht20Error Aht20Sensor::lastError() const {
  return lastError_;
}

uint32_t Aht20Sensor::consecutiveFailures() const {
  return consecutiveFailures_;
}

bool Aht20Sensor::initialized() const {
  return initialized_;
}

bool Aht20Sensor::probe() {
  // 发送空事务，仅用于确认 0x38 地址应答 / Send an empty transaction only to confirm address 0x38 responds
  wire_->beginTransmission(kAht20Address);
  if (wire_->endTransmission(true) != 0) {
    return fail(Aht20Error::NoResponse);
  }
  return true;
}

bool Aht20Sensor::readStatus(uint8_t& status) {
  // AHT20 通过单字节读取返回当前状态 / AHT20 returns the current status through a one-byte read
  const size_t requested = wire_->requestFrom(
      static_cast<uint8_t>(kAht20Address), static_cast<size_t>(1), true);
  if (requested != 1U || wire_->available() < 1) {
    return fail(Aht20Error::StatusReadFailed);
  }
  status = wire_->read();
  return true;
}

bool Aht20Sensor::ensureCalibration() {
  // 校准位未置位时发送初始化校准命令 / Send the calibration initialization command when the calibration bit is clear
  uint8_t status = 0;
  if (!readStatus(status)) {
    return false;
  }
  if ((status & kStatusCalibrationMask) != 0) {
    return true;
  }

  if (!sendCommand(kInitializeCommand, sizeof(kInitializeCommand))) {
    return false;
  }
  if (!waitUntilReady(kMeasurementTimeoutMs)) {
    return fail(Aht20Error::CalibrationFailed);
  }
  if (!readStatus(status) || (status & kStatusCalibrationMask) == 0) {
    return fail(Aht20Error::CalibrationFailed);
  }
  return true;
}

bool Aht20Sensor::sendCommand(const uint8_t* command, size_t length) {
  // 写入完整命令帧并检查 I2C 结束状态 / Write the complete command frame and check the I2C result
  wire_->beginTransmission(kAht20Address);
  if (wire_->write(command, length) != length ||
      wire_->endTransmission(true) != 0) {
    return fail(Aht20Error::WriteFailed);
  }
  return true;
}

bool Aht20Sensor::waitUntilReady(uint32_t timeoutMs) {
  // 轮询 Busy 位，避免固定延时后盲目读取 / Poll the Busy bit instead of blindly reading after a fixed delay
  const uint32_t startMs = millis();
  while (millis() - startMs < timeoutMs) {
    uint8_t status = 0;
    if (!readStatus(status)) {
      return false;
    }
    if ((status & kStatusBusyMask) == 0) {
      return true;
    }
    delay(kMeasurementPollIntervalMs);
  }
  return fail(Aht20Error::BusyTimeout);
}

bool Aht20Sensor::readFrame(uint8_t* frame, size_t length) {
  // 检查返回长度，避免解析不完整数据 / Check the returned length to avoid parsing an incomplete frame
  const size_t received = wire_->requestFrom(
      static_cast<uint8_t>(kAht20Address), static_cast<size_t>(length), true);
  if (received != length) {
    while (wire_->available()) {
      wire_->read();
    }
    return fail(Aht20Error::ShortRead);
  }
  for (size_t index = 0; index < length; ++index) {
    frame[index] = wire_->read();
  }
  return true;
}

bool Aht20Sensor::softReset() {
  // 软复位后等待器件恢复，再重新确认校准 / Wait for recovery after soft reset, then verify calibration again
  if (!sendCommand(kSoftResetCommand, sizeof(kSoftResetCommand))) {
    return false;
  }
  delay(20);
  return ensureCalibration();
}

bool Aht20Sensor::fail(Aht20Error error) {
  // 失败时保留上次数值，但标记为过期 / Retain the previous value but mark it as stale on failure
  lastError_ = error;
  measurement_.stale = true;
  ++consecutiveFailures_;

  const uint32_t nowMs = millis();
  if (consecutiveFailures_ >= kRecoveryFailureThreshold &&
      nowMs - lastRecoveryMs_ >= kRecoveryIntervalMs) {
    lastRecoveryMs_ = nowMs;
    softReset();
  }
  return false;
}

void Aht20Sensor::clearError() {
  lastError_ = Aht20Error::None;
}

uint8_t Aht20Sensor::calculateCrc(const uint8_t* data, size_t length) {
  // 使用多项式 0x31、初始值 0xFF 计算 CRC-8 / Calculate CRC-8 with polynomial 0x31 and initial value 0xFF
  uint8_t crc = 0xFF;
  for (size_t index = 0; index < length; ++index) {
    crc ^= data[index];
    for (uint8_t bit = 0; bit < 8; ++bit) {
      crc = (crc & 0x80U) != 0 ? static_cast<uint8_t>((crc << 1) ^ 0x31U)
                               : static_cast<uint8_t>(crc << 1);
    }
  }
  return crc;
}

Aht20Sensor aht20;

}  // OmiPetSensor 命名空间 / OmiPetSensor namespace
