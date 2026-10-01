#include "wifi_manager.h"

namespace OmiPetNetwork {

bool WifiManagerService::begin() {
  // 显式使用 STA 模式，避免正常联网时保持 AP+STA / Explicitly use STA mode instead of keeping AP+STA during normal operation
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);

  // 非阻塞模式只适用于配置门户；autoConnect 仍可能同步等待连接 / Non-blocking mode applies to the portal, while autoConnect can still wait synchronously
  manager_.setConfigPortalBlocking(false);
  manager_.setConfigPortalTimeout(kProvisioningTimeoutSeconds);
  manager_.setConnectTimeout(kWifiConnectTimeoutSeconds);

  // 只启动已保存凭据的异步连接，不在 setup 中调用 autoConnect / Start an asynchronous connection with saved credentials without calling autoConnect from setup
  WiFi.begin();
  connectStartedAtMs_ = millis();
  initialized_ = true;
  provisioning_ = false;
  portalStarted_ = false;
  return connected();
}

void WifiManagerService::update() {
  if (!initialized_) {
    return;
  }

  if (WiFi.status() == WL_CONNECTED) {
    provisioning_ = false;
    if (portalStarted_) {
      // 连接成功后停止配置门户，避免继续占用 AP 和 Web 资源 / Stop the portal after connection to release AP and web resources
      manager_.stopConfigPortal();
      portalStarted_ = false;
    }
    return;
  }

  if (!portalStarted_ &&
      millis() - connectStartedAtMs_ >= kInitialWifiConnectWindowMs) {
    // 超时后才启动门户，避免启动阶段被 Wi-Fi 连接阻塞 / Start the portal only after timeout so Wi-Fi cannot block startup
    portalStarted_ = manager_.startConfigPortal(kProvisioningSsid,
                                                kProvisioningPassword);
    provisioning_ = portalStarted_;
  }

  if (portalStarted_) {
    // 非阻塞模式必须在 loop 中持续处理网页请求 / Non-blocking mode must process web requests in loop
    manager_.process();
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
