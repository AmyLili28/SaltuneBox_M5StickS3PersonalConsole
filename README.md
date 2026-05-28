# M5StickS3 Personal Console Native

这是把原 UIFlow2/MicroPython 项目迁移到 Arduino/C++ 的工程，项目目录固定在 `D:\Codex\M5StickS3PersonalConsole`。资源使用 LittleFS，图片和音频放在 `data/`。

## 当前状态

- PlatformIO Core 位于 `D:\Codex\pio-py311`。
- PlatformIO 缓存、平台和工具链位于用户目录 `.platformio`。
- 本地库已放入 `lib/`：`M5Unified`、`M5GFX`、`ArduinoJson`。
- 当前默认上传端口为 `COM4`，脚本会优先自动寻找 ESP32-S3 下载口。
- 固件编译通过，LittleFS 镜像生成通过。
- 最近一次完整烧录包含 LittleFS 资源和固件；设备进入下载模式时会显示为 ESP32-S3 下载口。

## 编译和烧录

```powershell
cd D:\Codex\M5StickS3PersonalConsole
D:\Codex\pio-py311\Scripts\platformio.exe run
D:\Codex\pio-py311\Scripts\platformio.exe run -t buildfs
.\tools\flash.ps1
```

如果上传失败，长按设备按键进入下载模式，看到内部绿灯闪烁后重新运行 `.\tools\flash.ps1`。

## 已迁移功能

| 功能 | 状态 |
| --- | --- |
| Home | 联网后用 NTP 同步北京时间到 RTC，显示日期/星期/时间、IP 粗定位城市、电量百分比和电量条、A/B 进入菜单提示、底部署名；无有效 RTC 时会在 Home 继续重试 |
| Launcher | B 短按切换功能，A 进入功能，B 长按回 Home；顶部显示日期/时间与电量；功能页使用 `135x135` 图片资源，图片下方显示名称 |
| Clapping Music | 按 `PRODUCT_GUIDE.md` 恢复 12 步节奏游戏：Easy/Medium/Hard、Play/Practice、M5/YOU 双行节拍点、ACC 评分、Advance 进度、暂停/继续、Complete/Game Over |
| Voice Memo | `+ New rec` 与多条 `Memo` 列表、录音保存到 `/memo_0.wav` 到 `/memo_7.wav`、播放、长按 A 删除、录音时 B 停止保存 |
| Coin Flip | A 键或摇动随机正反面 |
| Woodfish | 计数保存、A 短按/摇动敲击、长按 A 清零、增强音量的 WAV 木鱼声、帧图动画 |
| RPS | A 键或摇动触发，进入后有防误触延迟，快速闪图后横向居中显示石头/剪刀/布结果图片和英文名称 |
| WiFi Setup | 显示 AP 名称、热点密码 `12345678`、访问地址、STA 连接状态；AP `M5StickS3-Setup`、DNS captive portal、网页配网；联网和外网检查成功后才保存，并同步 RTC 与 Home 城市 |
| Sandtimer | A 设置/启动/切换倒计时，B 切换主题，长按 A 取消，结束后提示 |
| Haiyan | C++ 实时海盐模拟，按时间切换背景，粒子参与物理，显示层合成为粉蓝半透明渐变水体，A 重置，短按 B 不退出，长按 B 回功能页 |
| Weather | 联网后 IP 粗定位，顶部居中显示城市，大号显示当前温度/天气图标，并列出 7 天最高/最低温和晴/阴/雨/雪状态；定位成功后把城市保存给 Home |

所有独立功能内，长按 B 返回功能选择页并停留在当前功能卡片；回到功能选择页后，短按 B 继续切换下一个功能，长按 B 返回 Home。

## 资源

主要资源路径：

- 功能页图片：`data/img/*.jpg`
- RPS 结果图片：`data/img/rps_rock.jpg`、`rps_scissors.jpg`、`rps_paper.jpg`
- 木鱼动画帧：`data/img/woodfish_frame0.jpg` 到 `woodfish_frame4.jpg`
- 木鱼音效：`data/audio/woodfish_knock.wav`
- Native Voice Memo 录音槽位：`/memo_0.wav` 到 `/memo_7.wav`，并兼容旧的 `/memo.wav`

注意：M5GFX 原生支持 JPG/PNG 文件显示，但不直接播放 PNG 动画。动画在代码里按帧切换图片实现。
