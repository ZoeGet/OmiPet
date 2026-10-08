#include "battery_monitor.h"

#include <cmath>

namespace OmiPetBattery {
namespace {
//  正常电池采样间隔 / Normal battery-sampling interval
constexpr uint32_t kSampleIntervalMs = 30000U;
//  超过此时间没有成功采样时使结果失效 / Invalidate the result after this long without a successful sample
constexpr uint32_t kStaleIntervalMs = 120000U;
//  每轮参与平均的目标采样数 / Target samples included in each average
constexpr uint8_t kSampleCount = 5;
//  单次采样失败后的重试次数 / Retries after an individual sample fails
constexpr uint8_t kReadAttempts = 3;
//  ADC 失败重试之间的短暂让步时间 / Short delay between ADC retries
constexpr uint32_t kRetryDelayMs = 4U;

//  电压和电量映射表中的一个节点 / One node in the voltage-to-charge mapping table
struct VoltagePercentPoint {
  float voltageV;
  uint8_t percent;
};

//  单节锂电静置电压估算表，带载或充电时仅作为近似值 / Single-cell Li-ion resting-voltage estimate; approximate under load or while charging
constexpr VoltagePercentPoint kVoltagePercentTable[] = {
    {3.30F, 0},   {3.50F, 5},   {3.60F, 10}, {3.70F, 20},
    {3.80F, 35},  {3.90F, 55},  {4.00F, 75}, {4.10F, 90},
    {4.20F, 100},
};
}

void BatteryMonitor::begin() {
  //  配置 ADC 输入并使用较高衰减范围覆盖电池分压电压 / Configure the ADC input with high attenuation for the divided battery voltage
  pinMode(kBatteryAdcPin, INPUT);
  analogSetPinAttenuation(kBatteryAdcPin, ADC_11db);
  initialized_ = true;
  //  在 Wi-Fi 初始化前尽早获取一次电池读数 / Get an early battery reading before Wi-Fi initialization
  update(true);
}

bool BatteryMonitor::update(bool force) {
  //  未初始化时拒绝采样，避免访问未配置的 ADC / Reject sampling before ADC initialization
  if (!initialized_) {
    return false;
  }
  const uint32_t nowMs = millis();
  //  限制采样频率，并在长期失败后标记旧结果失效 / Limit sampling frequency and invalidate an old result after prolonged failure
  if (!force && nowMs - lastAttemptMs_ < kSampleIntervalMs) {
    if (measurement_.valid && nowMs - measurement_.timestampMs > kStaleIntervalMs) {
      measurement_.valid = false;
      return true;
    }
    return false;
  }
  lastAttemptMs_ = nowMs;
  float voltageV = 0.0F;
  //  ADC2 被 Wi-Fi 占用时可能失败；失败值不能当作零伏 / ADC2 may be unavailable while Wi-Fi uses it; failure must not become zero volts
  if (!readVoltage(voltageV)) {
    measurement_.lastReadSucceeded = false;
    if (measurement_.valid && nowMs - measurement_.timestampMs > kStaleIntervalMs) {
      measurement_.valid = false;
      return true;
    }
    return false;
  }
  measurement_.voltageV = voltageV;
  measurement_.percent = estimatePercent(voltageV);
  measurement_.timestampMs = nowMs;
  measurement_.valid = true;
  measurement_.lastReadSucceeded = true;
  return true;
}

const BatteryMeasurement& BatteryMonitor::measurement() const {
  return measurement_;
}

bool BatteryMonitor::readVoltage(float& voltageV) {
  //  只用非零读数参与平均，避免 ADC 失败返回值污染结果 / Average only non-zero readings so failed ADC calls do not pollute the result
  uint32_t totalMilliVolts = 0;
  uint8_t successfulSamples = 0;
  for (uint8_t sampleIndex = 0; sampleIndex < kSampleCount; ++sampleIndex) {
    uint32_t adcMilliVolts = 0;
    for (uint8_t attempt = 0; attempt < kReadAttempts; ++attempt) {
      //  analogReadMilliVolts 已包含 eFuse 校准路径 / analogReadMilliVolts includes the eFuse calibration path
      adcMilliVolts = analogReadMilliVolts(kBatteryAdcPin);
      if (adcMilliVolts != 0U) {
        break;
      }
      delay(kRetryDelayMs);
    }
    if (adcMilliVolts == 0U) {
      continue;
    }
    totalMilliVolts += adcMilliVolts;
    ++successfulSamples;
    delay(2U);
  }
  if (successfulSamples < 3U) {
    //  有效样本不足时保留旧值，不生成新的电压 / Keep the old value when too few samples are valid
    return false;
  }
  const float adcVoltageV = static_cast<float>(totalMilliVolts) /
                            (1000.0F * static_cast<float>(successfulSamples));
  //  按硬件分压比例还原 VBAT / Reconstruct VBAT using the hardware divider ratio
  voltageV = adcVoltageV / kBatteryDividerRatio;
  //  拒绝明显越界值，避免断线或错误引脚显示成有效电量 / Reject implausible values from a disconnected or incorrect pin
  return std::isfinite(voltageV) && voltageV >= 2.5F && voltageV <= 5.0F;
}

uint8_t BatteryMonitor::estimatePercent(float voltageV) {
  //  节点之间线性插值；这是估算值，不替代电池燃料计 / Linearly interpolate between nodes; this is an estimate, not a fuel gauge
  constexpr size_t kPointCount = sizeof(kVoltagePercentTable) /
                                 sizeof(kVoltagePercentTable[0]);
  if (voltageV <= kVoltagePercentTable[0].voltageV) {
    return kVoltagePercentTable[0].percent;
  }
  for (size_t index = 1; index < kPointCount; ++index) {
    const VoltagePercentPoint& lower = kVoltagePercentTable[index - 1U];
    const VoltagePercentPoint& upper = kVoltagePercentTable[index];
    if (voltageV <= upper.voltageV) {
      const float fraction = (voltageV - lower.voltageV) /
                             (upper.voltageV - lower.voltageV);
      const float percent = static_cast<float>(lower.percent) +
                            fraction * static_cast<float>(upper.percent - lower.percent);
      return static_cast<uint8_t>(percent + 0.5F);
    }
  }
  return kVoltagePercentTable[kPointCount - 1U].percent;
}

BatteryMonitor battery;
}
