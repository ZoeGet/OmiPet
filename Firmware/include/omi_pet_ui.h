#pragma once

namespace OmiPetUi {

//  初始化桌面宠物界面 / Initialize the desktop pet user interface
void begin();

//  更新温湿度显示 / Update the temperature and humidity display
void setEnvironment(float temperatureC, float humidityPercent, bool valid);

//  更新 Wi-Fi 状态显示 / Update the Wi-Fi status display
void setNetworkStatus(bool connected, bool provisioning);

//  更新语音交互状态显示 / Update the voice interaction status display
void setVoiceStatus(const char* statusText);

//  更新时钟和动画 / Update the clock and animation
void update();

}  //  OmiPetUi 命名空间 / OmiPetUi namespace
