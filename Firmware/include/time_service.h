#pragma once

#include <Arduino.h>

namespace OmiPetTime {

struct DateTime {
  bool valid = false;
  int year = 0;
  int month = 0;
  int day = 0;
  int hour = 0;
  int minute = 0;
};

//  NTP 时间同步服务 / NTP time synchronization service
class TimeService {
 public:
  //  初始化时间同步状态 / Initialize time synchronization state
  void begin();

  //  根据当前 Wi-Fi 状态启动或维护 NTP 同步 / Start or maintain NTP synchronization according to Wi-Fi state
  void update(bool wifiConnected);

  //  获取当前可显示的本地时间 / Get the current displayable local time
  DateTime current() const;

 private:
  bool initialized_ = false;
  bool wifiConnected_ = false;
  bool configured_ = false;
  bool synchronized_ = false;
  uint32_t lastConfigAtMs_ = 0;
  DateTime current_;
};

extern TimeService ntp;

}  //  OmiPetTime 命名空间 / OmiPetTime namespace
