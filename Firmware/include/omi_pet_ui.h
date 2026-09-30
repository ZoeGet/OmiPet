#pragma once

namespace OmiPetUi {

// 初始化桌面宠物界面 / Initialize the desktop pet user interface
void begin();

// 更新温湿度显示 / Update the temperature and humidity display
void setEnvironment(float temperatureC, float humidityPercent, bool valid);

// 更新时钟和动画 / Update the clock and animation
void update();

}  // OmiPetUi 命名空间 / OmiPetUi namespace
