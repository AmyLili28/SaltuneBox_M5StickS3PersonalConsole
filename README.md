# Saltune Box

基于 M5StickS3 的个人音乐互动终端，使用 Arduino/C++ 和 PlatformIO 开发，集成节奏练习、录音、动作感应、小游戏与联网信息。

## 功能

| 模块 | 功能 |
| --- | --- |
| Home | 时钟、电量和 WiFi 状态 |
| Clapping Music | 多难度节奏练习与准确率统计 |
| Dino | 跳跃、趴下与最高分记录 |
| Voice Memo | WAV 录音、回放与删除 |
| Coin Flip / RPS | 按键或纵向敲击触发随机结果 |
| Woodfish | 敲击动画、音效与计数 |
| Sandtimer / Haiyan | 倾斜控制的沙漏与流体视觉 |
| Volume | 动作感应调节系统音量 |
| WiFi Setup | 热点配网与多网络管理 |
| Weather / Today History | 天气预报与音乐史事件 |
| Hidden Entry | 隐藏信箱互动 |

## 构建与烧录

准备 M5StickS3、USB 数据线，以及 [PlatformIO Core 6.x](https://docs.platformio.org/en/latest/core/installation/index.html) 或 VS Code 的 PlatformIO 扩展。Espressif32 固定为 `7.0.1`，第三方依赖随仓库保存在 `lib/`。

```sh
git clone https://github.com/AmyLili28/SaltuneBox_M5StickS3PersonalConsole.git
cd SaltuneBox_M5StickS3PersonalConsole
pio run
```

连接设备后，首次烧录需要同时上传 LittleFS 资源和固件：

```sh
pio run -t uploadfs
pio run -t upload
```

用 `pio device list` 查看串口；需要指定设备时，在命令末尾加 `--upload-port <PORT>`。Windows 也可运行 `./tools/flash.ps1 -UploadPort COMx`。

仅修改 C++ 时只需上传固件；修改 `data/` 后需重新上传 LittleFS。`uploadfs` 会重写文件系统并清除设备中的录音。

## 操作

- 主时钟页：短按 A 或 B 进入功能列表。
- 功能列表：B 切换，A 进入，长按 B 返回时钟。
- 功能内：A 执行主要操作，B 执行次要操作，多数页面长按 B 返回。

完整操作见[说明书](docs/manual/reading.pdf)。印刷使用[出血单页版](docs/manual/print.pdf)或[骑马钉拼版](docs/manual/booklet.pdf)。

## 目录

```text
src/              固件实现
include/          应用配置与流体模拟
data/             LittleFS 图片与音效
lib/              固定版本的第三方库及许可证
tools/            烧录与图片压缩工具
docs/             技术说明与最终版说明书
platformio.ini    构建配置
```

开发结构见[技术说明](docs/PROJECT_OVERVIEW.md)，更新见[CHANGELOG](CHANGELOG.md)。问题反馈请使用 [Issues](https://github.com/AmyLili28/SaltuneBox_M5StickS3PersonalConsole/issues)，并附上复现步骤、构建输出及设备信息。

## 版权

© 2026 刘译璘（Yilin Liu）。原创代码与素材保留全部权利，见 [COPYRIGHT.md](COPYRIGHT.md)；第三方组件遵循各自许可证。
