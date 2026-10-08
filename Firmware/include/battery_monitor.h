#pragma once

#include <Arduino.h>

namespace OmiPetBattery {

//  电池分压采样 GPIO；改版换脚时只需修改此常量 / Battery divider ADC GPIO; change only this constant when the PCB pin changes
constexpr uint8_t kBatteryAdcPin = 13;
//  BAT_ADC 对 VBAT 的分压比例：150 kΩ / (100 kΩ + 150 kΩ) / BAT_ADC-to-VBAT divider ratio: 150 kΩ / (100 kΩ + 150 kΩ)
constexpr float kBatteryDividerRatio = 0.6F;

//  最近一次电池采样结果 / Most recent battery measurement result
struct BatteryMeasurement {
  //  根据分压换算出的电池端电压 / Battery-terminal voltage reconstructed from the divider
  float voltageV = 0.0F;
  //  根据单节锂电电压表估算的电量 / Estimated charge from the single-cell Li-ion voltage table
  uint8_t percent = 0;
  //  最近一次成功采样的时间 / Timestamp of the most recent successful sample
  uint32_t timestampMs = 0;
  //  当前是否存在未过期的有效结果 / Whether a non-stale valid result is available
  bool valid = false;
  //  最近一次采样尝试是否成功 / Whether the most recent sampling attempt succeeded
  bool lastReadSucceeded = false;
};

//  电池 ADC 采样和电量估算服务 / Battery ADC sampling and charge-estimation service
class BatteryMonitor {
 public:
  //  初始化 ADC 并立即尝试首次采样 / Initialize the ADC and attempt the first sample immediately
  void begin();
  //  按周期采样；force=true 时忽略采样间隔 / Sample periodically; force=true bypasses the sample interval
  bool update(bool force = false);
  //  获取最近一次采样结果 / Get the most recent measurement
  const BatteryMeasurement& measurement() const;

 private:
  //  读取并平均多次 ADC 电压 / Read and average multiple ADC voltage samples
  bool readVoltage(float& voltageV);
  //  将电压转换为单节锂电的估算百分比 / Convert voltage to an estimated single-cell Li-ion percentage
  static uint8_t estimatePercent(float voltageV);
  //  保存采样状态 / Stored sampling state
  BatteryMeasurement measurement_;
  //  上一次采样尝试时间 / Time of the last sampling attempt
  uint32_t lastAttemptMs_ = 0;
  //  是否已经完成初始化 / Whether initialization has completed
  bool initialized_ = false;
};

//  全局电池监测实例 / Global battery-monitor instance
extern BatteryMonitor battery;

}  //  OmiPetBattery 命名空间 / OmiPetBattery namespace
