#include "wifi_manager.h"

//  Wi-Fi 配网、重连和热点管理实现 / Wi-Fi provisioning, reconnection, and portal management implementation

namespace OmiPetNetwork {

//  初始化 Wi-Fi 状态和配网服务 / Initialize Wi-Fi state and provisioning service
bool WifiManagerService::begin() {
  //  显式使用 STA 模式，避免正常联网时保持 AP+STA / Explicitly use STA mode instead of keeping AP+STA during normal operation
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);

  //  非阻塞模式只适用于配置门户；autoConnect 仍可能同步等待连接 / Non-blocking mode applies to the portal, while autoConnect can still wait synchronously
  manager_.setConfigPortalBlocking(false);
  manager_.setConfigPortalTimeout(kProvisioningTimeoutSeconds);
  manager_.setConnectTimeout(kWifiConnectTimeoutSeconds);

  //  只启动已保存凭据的异步连接，不在 setup 中调用 autoConnect / Start an asynchronous connection with saved credentials without calling autoConnect from setup
  WiFi.begin();
  connectStartedAtMs_ = millis();
  lastReconnectAttemptMs_ = connectStartedAtMs_;
  disconnectedAtMs_ = 0;
  initialized_ = true;
  provisioning_ = false;
  portalStarted_ = false;
  everConnected_ = false;
  return connected();
}

//  驱动连接重试和配网状态机 / Advance connection retries and the provisioning state machine
void WifiManagerService::update() {
  if (!initialized_) {
    return;
  }

  if (WiFi.status() == WL_CONNECTED) {
    everConnected_ = true;
    disconnectedAtMs_ = 0;
    provisioning_ = false;
    if (portalStarted_) {
      //  连接成功后停止配置门户，避免继续占用 AP 和 Web 资源 / Stop the portal after connection to release AP and web resources
      manager_.stopConfigPortal();
      portalStarted_ = false;
    }
    return;
  }

  if (portalStarted_) {
    //  配网门户运行期间只处理网页请求，不重复发起连接流程 / Process the portal without starting another connection flow
    manager_.process();
    return;
  }

  if (!everConnected_ &&
      millis() - connectStartedAtMs_ >= kInitialWifiConnectWindowMs) {
    //  超时后才启动门户，避免启动阶段被 Wi-Fi 连接阻塞 / Start the portal only after timeout so Wi-Fi cannot block startup
    portalStarted_ = manager_.startConfigPortal(kProvisioningSsid,
                                                kProvisioningPassword);
    provisioning_ = portalStarted_;
    return;
  }

  if (everConnected_) {
    const uint32_t nowMs = millis();
    if (disconnectedAtMs_ == 0U) {
      disconnectedAtMs_ = nowMs;
      Serial.println("[WIFI] connection lost, retry window started");
    }

    if (nowMs - disconnectedAtMs_ >= kWifiReconnectGracePeriodMs) {
      //  重连超时后开启配网门户 / Start provisioning after the reconnect grace period expires
      portalStarted_ = manager_.startConfigPortal(kProvisioningSsid,
                                                  kProvisioningPassword);
      provisioning_ = portalStarted_;
      return;
    }

    if (nowMs - lastReconnectAttemptMs_ >= kWifiReconnectIntervalMs) {
      //  在宽限期内主动重连，不立即开启配网热点 / Retry during the grace period without opening the portal immediately
      lastReconnectAttemptMs_ = nowMs;
      Serial.println("[WIFI] disconnected, reconnecting");
      if (!WiFi.reconnect()) {
        //  某些断线状态下 reconnect 可能失败，使用已保存凭据重新启动连接 / Restart the connection with saved credentials when reconnect fails
        WiFi.begin();
      }
    }
  }
}

bool WifiManagerService::connected() const {
  return WiFi.status() == WL_CONNECTED;
}

bool WifiManagerService::provisioning() const {
  return provisioning_;
}

WifiManagerService wifi;

}  //  OmiPetNetwork 命名空间 / OmiPetNetwork namespace
