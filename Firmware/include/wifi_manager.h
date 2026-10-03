#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>

namespace OmiPetNetwork {

//  配网热点配置 / Provisioning access-point configuration
constexpr char kProvisioningSsid[] = "OmiPet-Setup";  //  配网门户 SoftAP 名称 / Provisioning portal SoftAP name
constexpr char kProvisioningPassword[] = "omipet123";  //  配网门户 SoftAP 密码 / Provisioning portal SoftAP password
constexpr uint32_t kProvisioningTimeoutSeconds = 180;  //  配网门户最长运行时间 / Maximum provisioning portal duration
constexpr uint32_t kWifiConnectTimeoutSeconds = 20;  //  单次 Wi-Fi 连接超时时间 / Timeout for one Wi-Fi connection attempt
constexpr uint32_t kInitialWifiConnectWindowMs = 5000;  //  启动阶段等待联网的窗口 / Startup window for the initial Wi-Fi connection
constexpr uint32_t kWifiReconnectIntervalMs = 10000;  //  两次 Wi-Fi 重连尝试之间的间隔 / Interval between Wi-Fi reconnect attempts
constexpr uint32_t kWifiReconnectGracePeriodMs = 60000;  //  断网后进入配网回退前的宽限时间 / Grace period before provisioning fallback after disconnect

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
