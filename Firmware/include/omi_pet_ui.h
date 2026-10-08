#pragma once

#include <stdint.h>

namespace OmiPetUi {

//  初始化桌面仪表盘界面 / Initialize the desktop dashboard user interface
void begin();

//  更新温湿度显示 / Update the temperature and humidity display
void setEnvironment(float temperatureC, float humidityPercent, bool valid);
//  更新电池电压和估算电量显示 / Update the battery voltage and estimated charge display
void setBatteryStatus(float voltageV, uint8_t percent, bool valid);

//  更新 Wi-Fi 状态和名称显示 / Update the Wi-Fi status and name display
void setNetworkStatus(bool connected, bool provisioning, const char* ssid);

//  更新联网时间和日期显示 / Update the network time and date display
void setDateTime(bool valid, int year, int month, int day, int hour, int minute);

//  更新语音交互状态显示 / Update the voice interaction status display
void setVoiceStatus(const char* statusText);

//  按秒更新时钟 / Update the clock once per second
void update();

}  //  OmiPetUi 命名空间 / OmiPetUi namespace
