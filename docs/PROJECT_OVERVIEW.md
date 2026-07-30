# Saltune Box 技术概览

## 1. 项目范围

Saltune Box 是运行在 M5StickS3 上的单固件多应用系统。项目使用 Arduino/C++ 和 PlatformIO 开发，通过统一的页面状态、输入处理和资源管理，在有限的屏幕、Flash 与内存条件下提供多个独立功能。

本文只描述公开技术结构，不包含私人内容、设备凭据或隐藏入口的具体文本。

## 2. 运行架构

固件采用一个主循环和多个应用状态：

1. 更新按键、IMU、WiFi Portal 和电源状态。
2. 根据当前应用分派输入事件。
3. 更新当前应用的数据与动画。
4. 仅在需要时刷新显示，降低无效绘制和功耗。
5. 使用 Preferences / NVS 保存轻量状态，使用 LittleFS 保存资源和录音。

不同应用共享显示、声音、动作识别、网络和持久化能力，但各自保留独立的进入、运行和退出状态。

## 3. 主要模块

### 固件入口

`src/main.cpp` 包含设备初始化、主循环、应用调度以及当前各应用实现。项目仍采用单固件结构，后续重构需要以实机回归测试为前提。

### 应用配置

`include/AppConfig.h` 保存屏幕尺寸、颜色、应用枚举、功能名称和 Launcher 图标路径。功能顺序或资源映射发生变化时，应同步更新该文件。

### 流体模拟

`include/HaiyanSim.h` 封装 Haiyan 页面使用的粒子状态和重力方向更新逻辑。

### 运行资源

`data/` 会在 `uploadfs` 阶段写入 LittleFS：

- `data/img/`：Launcher 图标、游戏图片和动画帧。
- `data/audio/`：设备音效。

新设备只上传固件不会获得这些资源，因此首次部署必须同时上传 LittleFS。

### 第三方依赖

`lib/` 当前保存项目验证过的 M5Unified、M5GFX 和 ArduinoJson 版本。各组件继续遵循其目录内的原始许可证。

## 4. 输入与设备能力

- Button A：确认、触发或开始。
- Button B：切换、次要操作或返回。
- IMU：识别纵向敲击、反向敲击、左右倾斜和设备姿态。
- Microphone：采集单声道 PCM 数据并写入 WAV 文件。
- Speaker：播放录音和应用音效。
- RTC / NTP：维护本地时间并在联网后校准。
- WiFi：提供配置入口、时间同步和联网内容。

动作识别由多个应用共享。修改阈值、方向或去抖逻辑时，需要分别回归 Coin Flip、Woodfish、RPS、Volume、Sandtimer 和 Haiyan。

## 5. 存储策略

| 存储 | 内容 |
| --- | --- |
| 固件 Flash | C++ 程序和编译期常量。 |
| LittleFS | 图片、音效和用户录音。 |
| Preferences / NVS | 音量、分数、计数和 WiFi 配置。 |

录音可用时长取决于 LittleFS 分区大小及当前资源占用，不应在公开文档中写成永久固定值。

## 6. 构建与部署

项目的可移植构建配置位于根目录 `platformio.ini`。

```powershell
pio run
```

完整部署顺序：

```powershell
pio run -t uploadfs --upload-port COMx
pio run -t upload --upload-port COMx
```

Windows 下也可以使用 `tools/flash.ps1` 自动识别设备并执行完整部署。

## 7. 公开仓库边界

应提交：

- 固件源码和头文件。
- 设备运行必需的图片、音频和分区配置。
- 可移植的构建配置和维护工具。
- README、CHANGELOG、技术文档与第三方许可证。

不应提交：

- WiFi 凭据、API 密钥、访问令牌或用户录音。
- 固定串口、本机绝对路径和个人开发环境目录。
- `.pio/`、编译缓存、日志、临时文件和 IDE 本地配置。
- 仅用于设计参考但没有再分发授权的图片。
- 本地生成的 Word、PDF 和诊断导出文件，除非明确作为正式发布物。

## 8. 维护原则

- 修改固件逻辑后至少完成一次 PlatformIO 全量构建。
- 修改 `data/` 后同时检查 LittleFS 占用并在实机重新上传文件系统。
- 修改录音或扬声器逻辑时检查麦克风与扬声器资源切换。
- 修改动作识别时逐项测试所有依赖 IMU 的应用。
- 新增功能或修复用户可见问题时同步更新 `CHANGELOG.md`。
