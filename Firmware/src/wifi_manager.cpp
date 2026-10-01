#include "wifi_manager.h"

namespace OmiPetNetwork {

bool WifiManagerService::begin() {
  // 显式使用 STA 模式，避免正常联网时保持 AP+STA / Explicitly use STA mode instead of keeping AP+STA during normal operation
  WiFi.mode(WIFI_STA);

  // 非阻塞模式允许 LCD、传感器和蜂鸣器继续运行 / Non-blocking mode lets the LCD, sensor, and buzzer continue running
  manager_.setConfigPortalBlocking(false);
  manager_.setConfigPortalTimeout(kProvisioningTimeoutSeconds);
  manager_.setConnectTimeout(kWifiConnectTimeoutSeconds);

  // 自动连接已保存的 Wi-Fi；失败时启动 OmiPet-Setup 配网热点 /
  // Connect to saved Wi-Fi automatically; start the OmiPet-Setup portal on failure
  const bool connected =
      manager_.autoConnect(kProvisioningSsid, kProvisioningPassword);
  initialized_ = true;
  provisioning_ = !connected;
  return connected;
}

void WifiManagerService::update() {
  if (!initialized_) {
    return;
  }

  // 非阻塞模式必须在 loop 中持续处理网页请求 / Non-blocking mode must process web requests from loop
  manager_.process();
  if (WiFi.status() == WL_CONNECTED) {
    provisioning_ = false;
  }
}

bool WifiManagerService::connected() const {
  return WiFi.status() == WL_CONNECTED;
}

bool WifiManagerService::provisioning() const {
  return provisioning_;
}

WifiManagerService wifi;

}  // OmiPetNetwork 命名空间 / OmiPetNetwork namespace
