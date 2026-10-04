#include "time_service.h"

//  NTP 时间同步实现 / NTP time synchronization implementation

#include <time.h>

namespace OmiPetTime {
namespace {

constexpr char kTimeZone[] = "UTC-8";  //  中国标准时间 UTC+8 / China Standard Time UTC+8
constexpr uint32_t kNtpRetryIntervalMs = 10000U;  //  NTP 配置重试间隔 / NTP configuration retry interval
constexpr time_t kMinimumValidEpoch = 1704067200;  //  2024-01-01 00:00:00 UTC / 2024-01-01 00:00:00 UTC

}  //  匿名命名空间 / Anonymous namespace

//  初始化时间同步状态 / Initialize time synchronization state
void TimeService::begin() {
  initialized_ = true;
  wifiConnected_ = false;
  configured_ = false;
  synchronized_ = false;
  lastConfigAtMs_ = 0;
  current_ = DateTime{};
}

//  根据 Wi-Fi 状态启动或维护 NTP 同步 / Start or maintain NTP synchronization according to Wi-Fi state
void TimeService::update(bool wifiConnected) {
  if (!initialized_) {
    return;
  }

  if (!wifiConnected) {
    wifiConnected_ = false;
    configured_ = false;
    synchronized_ = false;
    current_.valid = false;
    return;
  }

  if (!wifiConnected_) {
    wifiConnected_ = true;
    configured_ = false;
    synchronized_ = false;
  }

  const uint32_t nowMs = millis();
  if (!configured_ || nowMs - lastConfigAtMs_ >= kNtpRetryIntervalMs) {
    configTzTime(kTimeZone, "ntp.aliyun.com", "pool.ntp.org",
                 "time.nist.gov");
    configured_ = true;
    lastConfigAtMs_ = nowMs;
    if (!synchronized_) {
      Serial.println("[TIME] NTP sync started");
    }
  }

  const time_t now = time(nullptr);
  if (now < kMinimumValidEpoch) {
    current_.valid = false;
    return;
  }

  struct tm localTime = {};
  if (localtime_r(&now, &localTime) == nullptr) {
    current_.valid = false;
    return;
  }

  current_.valid = true;
  current_.year = localTime.tm_year + 1900;
  current_.month = localTime.tm_mon + 1;
  current_.day = localTime.tm_mday;
  current_.hour = localTime.tm_hour;
  current_.minute = localTime.tm_min;
  if (!synchronized_) {
    synchronized_ = true;
    Serial.printf("[TIME] NTP synchronized %04d-%02d-%02d %02d:%02d\n",
                  current_.year, current_.month, current_.day,
                  current_.hour, current_.minute);
  }
}

//  获取当前可显示的本地时间 / Get the current displayable local time
DateTime TimeService::current() const {
  if (!wifiConnected_ || !synchronized_) {
    return DateTime{};
  }
  return current_;
}

TimeService ntp;

}  //  OmiPetTime 命名空间 / OmiPetTime namespace
