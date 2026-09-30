# 技术说明

## 固件结构

| 文件 | 职责 |
| --- | --- |
| [`src/main.cpp`](../src/main.cpp) | 初始化、主循环、输入分派和各应用实现 |
| [`include/AppConfig.h`](../include/AppConfig.h) | 应用顺序、颜色、布局尺寸和图标路径 |
| [`include/HaiyanSim.h`](../include/HaiyanSim.h) | Haiyan 粒子模拟与绘制 |

主循环更新按键、IMU、网络、音频和当前应用，按需刷新显示。各应用共享屏幕、声音、动作识别和持久化状态。

## 存储与联网

| 存储 | 内容 |
| --- | --- |
| 固件 Flash | C++ 程序与编译期常量 |
| LittleFS | `data/` 中的图片、音效，以及设备生成的 WAV 录音 |
| Preferences / NVS | 音量、分数、计数、位置缓存和 WiFi 配置 |

录音容量取决于 LittleFS 剩余空间。WiFi 配置和录音保存在设备本地；Weather 通过 IP 粗定位获取天气，Today History 在网络不可用时使用本地音乐史内容。

## 依赖

依赖源码保存在 `lib/`，构建配置见 [`platformio.ini`](../platformio.ini)。

| 组件 | 版本 | 许可证 |
| --- | --- | --- |
| M5Unified | 0.2.16 | [MIT](../lib/M5Unified/LICENSE) |
| M5GFX | 0.2.22 | [MIT](../lib/M5GFX/LICENSE)，附带字体按目录内声明授权 |
| ArduinoJson | 7.4.3 | [MIT](../lib/ArduinoJson/LICENSE.txt) |

## 修改与验证

```sh
pio run
pio run -t buildfs
```

构建与烧录步骤见 [README](../README.md#构建与烧录)。修改图片后检查路径与 LittleFS 占用；修改音频或动作识别后，在实机验证录音/回放及相关 IMU 应用。

## 说明书

`manual/` 保留 V6 最终版 PDF：阅读版 `reading.pdf`、带出血单页 `print.pdf`、骑马钉拼版 `booklet.pdf`。[content.md](manual/content.md) 保留可编辑的正文底稿；V6 排版源文件未收录。
