# OmiPet

OmiPet 是一个面向陪伴场景的桌面宠物相关的软硬件项目。仓库按功能划分目录，固件、硬件资料和 3D 文件分别独立管理。

## 项目目录

```text
OmiPet/
├─ Firmware/       # ESP32-S3 固件（PlatformIO + Arduino）
├─ Hardware/       # 硬件设计资料（后续添加）
├─ 3D/             # 3D 模型、结构设计文件（后续添加）
├─ README.md
└─ .gitignore
```

## 固件

固件位于 `Firmware/`，当前使用：

- PlatformIO
- Espressif32
- Arduino framework
- ESP32-S3 DevKitC-1

进入固件目录后编译：

```powershell
cd Firmware
pio run
```

编译产物位于 `Firmware/.pio/`，该目录已加入 Git 忽略规则，不会提交到仓库。

## 当前注意事项

`Firmware/platformio.ini` 中配置了 16MB Flash 和 PSRAM。

## 提交规范

请勿提交以下内容：

- PlatformIO 构建目录和生成的临时文件
- VS Code 本地配置及用户路径
- 本地环境变量、密码、Token、私钥和证书
- 串口日志、调试转储和个人临时文件

相关规则见根目录 `.gitignore`。
