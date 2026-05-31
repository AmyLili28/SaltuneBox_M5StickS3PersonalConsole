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

## A6 说明书小册子

说明书排版源文件放在 `src/index.html` 和 `src/styles.css`，导出脚本是 `src/generate.py`。最终文件会生成到 `dist/`：

- `dist/manual_A6_single_pages.pdf`：A6 单页顺序预览版。
- `dist/manual_A6_print_ready.pdf`：带 3mm 出血和裁切标记的打印版。
- `dist/manual_A6_preview.png`：封面和内页预览图。

最终版《Saltune Box》红黑说明书保存在 `dist/final_manual/`：

- `Saltune_Box_V6_red_black_reading_preview_64x98mm_P24fix.pdf`：阅读预览版。
- `Saltune_Box_V6_red_black_print_bleed_pages_70x104mm_P24fix.pdf`：单页带出血打印版。
- `Saltune_Box_V6_red_black_saddle_stitch_imposed_bleed_P24fix.pdf`：骑马钉拼版带出血版。

重新导出：

```powershell
python src\generate.py
```

更详细的文字替换、手写字体替换和图片替换说明见 `docs/manual_A6_README.md`。封面手写字体会优先读取 `fonts/handwriting.ttf`，没有该文件时使用系统手写感 fallback。

## 已迁移功能

| 功能 | 状态 |
| --- | --- |
| Home | 联网后用 NTP 同步北京时间到 RTC，居中显示日期/星期、大号时间、时间上方电池条、WiFi 状态、YR 标志、`Don't be afraid`、`Only for Zion Yan` 与 `Designated by YIlin`；无有效 RTC 时会在 Home 继续重试 |
| Launcher | B 短按切换功能，A 进入功能，B 长按回 Home；顶部显示日期/时间与电量；功能页使用 `135x135` 图片资源，图片下方显示名称 |
| Clapping Music | 按 `PRODUCT_GUIDE.md` 恢复 12 步节奏游戏：Easy/Medium/Hard、Play/Practice、M5/YOU 双行节拍点、ACC 评分、Advance 进度、暂停/继续、Complete/Game Over；当前版本放宽节拍判定、按下 A 即刻记录输入，并把黄色显示统一替换为粉色 |
| Voice Memo | `+ New rec` 与多条 `Memo` 列表、录音保存到 `/memo_0.wav` 到 `/memo_7.wav`、播放、长按 A 删除、录音时 B 停止保存 |
| Coin Flip | A 键或摇动随机正反面 |
| Woodfish | 计数保存、A 短按/摇动敲击、长按 A 清零、增强音量的 WAV 木鱼声、帧图动画 |
| RPS | A 键或摇动触发，进入后有防误触延迟，快速闪图后横向居中显示石头/剪刀/布结果图片和英文名称 |
| WiFi Setup | 显示 AP 名称、热点密码 `12345678`、访问地址、STA 连接状态；AP `M5StickS3-Setup`、DNS captive portal、网页配网；联网和外网检查成功后保存到最多 5 个 WiFi 列表，B 切换已保存网络，A 连接，长按 A 删除，并同步 RTC 与 Home 城市 |
| Sandtimer | 进入后初始为 30 秒，用重力左右晃动选择 0 到 60 分钟倒计时，每次 30 秒步进；A 确认进入沙漏倒计时，运行中 A 暂停、B 切换主题，暂停后长按 B 回时间选择页 |
| Haiyan | C++ 实时海盐模拟，按时间切换背景，交换 IMU X/Y 轴后按 bottle-of-ocean 风格重力映射，让水体始终流向屏幕里的新瓶底，A 重置，短按 B 不退出，长按 B 回功能页 |
| Weather | 联网后 IP 粗定位，顶部居中显示城市，大号显示当前温度/天气图标，并列出 7 天最高/最低温和晴/阴/雨/雪状态；定位成功后把城市保存给 Home |
| Today History | 进入后按当前 RTC 日期自动读取 Wikimedia `On this day`，优先筛选音乐史/摇滚相关事件，并从事件、出生、去世列表中补充音乐人条目；API 不通时使用少量本地音乐史兜底；A 刷新，B 切换 |
| Dino | Google 离线小恐龙风格游戏：A 跳跃，B 短按/按住趴下，趴下最长持续约 1.8 秒；暂停/入口/结束页长按 B 回功能页；结束页 A 再来一局、B 回入口页，最高分保存到 NVS |

所有独立功能内，长按 B 返回功能选择页并停留在当前功能卡片；回到功能选择页后，短按 B 继续切换下一个功能，长按 B 返回 Home。

## 资源

主要资源路径：

- 功能页图片：`data/img/*.jpg`
- RPS 结果图片：`data/img/rps_rock.jpg`、`rps_scissors.jpg`、`rps_paper.jpg`
- 木鱼动画帧：`data/img/woodfish_frame0.jpg` 到 `woodfish_frame4.jpg`
- 木鱼音效：`data/audio/woodfish_knock.wav`
- Today History 功能页图片：`data/img/history.jpg`
- Dino 功能页图片：`data/img/dino.jpg`
- Home YR 标志：`data/img/yr_logo.jpg`
- Native Voice Memo 录音槽位：`/memo_0.wav` 到 `/memo_7.wav`，并兼容旧的 `/memo.wav`

注意：当前固件只打包和显示 JPG 图片。动画在代码里按 JPG 帧切换图片实现，避免把未使用的 PNG 解码器编进固件。
