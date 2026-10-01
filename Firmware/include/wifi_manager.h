#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>

namespace OmiPetNetwork {

//  配网热点配置 / Provisioning access-point configuration
constexpr char kProvisioningSsid[] = "OmiPet-Setup";
constexpr char kProvisioningPassword[] = "omipet123";
constexpr uint32_t kProvisioningTimeoutSeconds = 180;
constexpr uint32_t kWifiConnectTimeoutSeconds = 20;
constexpr uint32_t kInitialWifiConnectWindowMs = 5000;
constexpr uint32_t kWifiReconnectIntervalMs = 10000;
constexpr uint32_t kWifiReconnectGracePeriodMs = 60000;

//  WiFiManager 非阻塞配网封装 / Non-blocking WiFiManager provisioning wrapper
class WifiManagerService {
 public:
  //  初始化 STA 连接，并在失败时启动配网门户 / Initialize STA connection and start the portal on failure
  bool begin();

  //  处理 Captive Portal 请求和 Wi-Fi 状态 / Process Captive Portal requests and Wi-Fi state
  void update();

  //  查询当前是否已经连接路由器 / Check whether the router is connected
  bool connected() const;

  //  查询是否正在等待网页配网 / Check whether web provisioning is active
  bool provisioning() const;

 private:
  WiFiManager manager_;
  uint32_t connectStartedAtMs_ = 0;
  uint32_t lastReconnectAttemptMs_ = 0;
  uint32_t disconnectedAtMs_ = 0;
  bool initialized_ = false;
  bool provisioning_ = false;
  bool portalStarted_ = false;
  bool everConnected_ = false;
};

extern WifiManagerService wifi;

}  //  OmiPetNetwork 命名空间 / OmiPetNetwork namespace
