# Saltune Box

基于 M5StickS3 的个人音乐互动终端。

Saltune Box 是一个使用 Arduino/C++ 与 PlatformIO 开发的 ESP32-S3 固件项目。它将节奏练习、录音备忘、动作感应、轻量游戏、联网信息和动态视觉整合在一台便携设备中，并使用 LittleFS 管理运行资源。

> 本仓库用于项目展示与技术学习。原创代码、界面、图像、音频及文档保留全部权利，具体见 [COPYRIGHT.md](COPYRIGHT.md)。

## 项目特点

- 针对 M5StickS3 的 128 x 240 像素屏幕和双按键交互设计。
- 使用 IMU 实现敲击、倾斜和重力方向感应。
- 使用内置麦克风与扬声器实现录音、回放和音效。
- 使用 LittleFS 分离固件与图片、音频等运行资源。
- 使用 Preferences / NVS 保存设置、分数和网络配置。
- 支持 RTC、NTP 时间同步以及 WiFi 联网功能。

## 功能概览

| 模块 | 说明 |
| --- | --- |
| Home | 日期、时间、电量和网络状态显示。 |
| Clapping Music | 多难度节奏练习与准确率统计。 |
| Dino | 支持跳跃、趴下和最高分保存的轻量游戏。 |
| Voice Memo | 本地 WAV 录音、播放和删除。 |
| Coin Flip | 按键或纵向敲击触发的随机硬币结果。 |
| Woodfish | 支持音效和计数保存的电子木鱼。 |
| RPS | 按键或纵向敲击触发的石头剪刀布。 |
| Sandtimer | 通过左右倾斜设置时长的重力沙漏。 |
| Haiyan | 根据设备姿态变化的瓶中流体视觉效果。 |
| Volume | 使用方向敲击逐级调节系统音量。 |
| WiFi Setup | AP 配网及多网络本地管理。 |
| Weather | 基于联网信息显示天气。 |
| Today History | 音乐与摇滚主题的历史事件，支持离线兜底。 |
| Hidden Entry | 保留内容说明的最终交互入口。 |

## 硬件与技术

| 类别 | 内容 |
| --- | --- |
| 目标设备 | M5StickS3 / ESP32-S3 |
| 开发框架 | Arduino / C++ |
| 构建工具 | PlatformIO |
| 设备与图形 | M5Unified、M5GFX |
| 数据解析 | ArduinoJson |
| 文件系统 | LittleFS |
| 本地存储 | Preferences / NVS |
| 设备能力 | LCD、按键、IMU、麦克风、扬声器、RTC、WiFi |

## 基本操作

- 电源键短按：开机或唤醒。
- 电源键长按：进入待机 / 低功耗状态。
- 电源键快速双击：关机。
- 主时钟页短按 A 或 B：进入功能选择页。
- 功能选择页短按 B：切换功能。
- 功能选择页短按 A：进入当前功能。
- 多数功能内长按 B：返回上一级。

各功能的完整操作说明见项目说明书。README 仅保留公开项目所需的概要信息。

## 构建环境

建议使用 PlatformIO Core 6.x 或安装了 PlatformIO 扩展的 Visual Studio Code。本项目将 Espressif32 平台固定为 `7.0.1`，第三方库当前保存在 `lib/` 中，以保持现有固件构建的一致性。

```powershell
pio run
```

如需指定串口，可将 `COMx` 替换为设备实际端口：

```powershell
pio run -t upload --upload-port COMx
```

## 完整烧录

新设备首次部署需要同时烧录固件和 LittleFS 资源：

```powershell
pio run -t uploadfs --upload-port COMx
pio run -t upload --upload-port COMx
```

Windows 用户也可以运行通用烧录脚本。脚本会尝试识别 ESP32-S3 串口，并依次上传文件系统和固件：

```powershell
.\tools\flash.ps1
```

如果只修改了 C++ 逻辑，可以仅上传固件；如果修改了 `data/` 中的图片或音频，则必须重新上传 LittleFS。

## 项目结构

| 路径 | 用途 |
| --- | --- |
| `src/main.cpp` | 固件主循环、页面状态和功能实现。 |
| `include/AppConfig.h` | 应用枚举、颜色、尺寸和功能图标配置。 |
| `include/HaiyanSim.h` | Haiyan 流体视觉模拟。 |
| `data/img/` | 设备运行所需的 LittleFS 图片。 |
| `data/audio/` | 设备运行所需的 LittleFS 音频。 |
| `lib/` | 固定版本的第三方依赖及其许可证。 |
| `tools/` | 烧录和资源优化辅助工具。 |
| `docs/` | 技术说明与项目文档。 |
| `CHANGELOG.md` | 功能迭代和修复记录。 |

## 数据与隐私

- 仓库不包含私人 WiFi 凭据、录音文件、API 密钥或访问令牌。
- 用户填写的 WiFi 信息仅保存在设备本地 NVS。
- Voice Memo 录音仅保存在设备本地 LittleFS。
- Weather 不读取 GPS，位置结果仅为基于网络信息的粗略判断。
- 网络内容不可用时，Today History 会使用本地音乐史内容。

## 项目状态

当前固件面向 M5StickS3 实机维护。更新内容见 [CHANGELOG.md](CHANGELOG.md)，技术结构见 [docs/PROJECT_OVERVIEW.md](docs/PROJECT_OVERVIEW.md)。

## 权利说明

Saltune Box 的原创代码、产品名称、界面设计、图片、音频和文档由刘译璘保留全部权利。未经书面许可，不得复制、修改、再发布或用于商业用途。

`lib/` 中的第三方组件不属于上述原创内容，其使用分别受组件目录内原许可证约束。
