# 更新日志

### 2026-05-28 17:05 +0800
- Weather 页面改为顶部居中城市、大号当前温度、天气图标和 7 天温度/天气列表；IP 定位失败时显示 `Location failed`，不再误显示北京。
- Haiyan 显示层改为粉蓝半透明密度渐变水体，保留内部粒子物理但不再直接显示松散粒子点。
- RPS 结果和随机闪图图片改为横向居中，闪图间隔缩短以提升随机滚动速度。
- Woodfish 木鱼 WAV 播放音量提高，fallback 音色同步提高音量。

### 2026-05-28 15:55 +0800
- 修复时间显示：主页面和功能页顶部时间现在优先读硬件 RTC，失败时使用联网后的 ESP32 系统时间兜底。
- NTP 同步后改为显式写入 M5Unified RTC 日期/时间结构，减少 RTC 写入不生效导致的 --:--。

## 2026-05-28 14:53 +08:00

- Home 去掉北京硬编码，改为显示天气/IP 定位成功后缓存的城市；启动时会读取上次保存的城市。
- NTP 同步改为在 WiFi 自动连接成功、配网页保存成功、天气刷新和 Home 无有效 RTC 时重试写入 RTC。
- 功能选择页顶部补回日期/时间与电量状态栏，和原 UIFlow2 版功能选择页的信息密度对齐。
- WiFi Setup 和 Weather 联网成功后会刷新 Home 城市缓存，避免天气页已经定位但主页面仍显示默认地点。

## 2026-05-28 10:14 +08:00

- WiFi Setup 设备端页面改为每秒刷新连接状态，显示 `ONLINE` / `JOINED` / `AP SETUP`、STA SSID/IP、已保存 SSID、AP 密码、访问地址和 Internet 检测结果。
- WiFi 配网页保存成功后，设备端同步显示 `Saved + online` 与 STA IP，避免手机端显示成功但 M5StickS3 页面看不出是否更新。
- 主页面时钟改为启动时等待更久自动连接已保存 WiFi 并同步 RTC，进入 Home 后每秒刷新时间，不再只是静态占位。
- Weather 增加 HTTPS 支持、IP 粗定位双接口兜底和 Open-Meteo HTTP/HTTPS 双路径；后续已改为定位失败时直接显示失败状态，不再回退北京。

## 2026-05-28 09:40 +08:00

- WiFi Setup 的联网验证从单一 Google 204 地址改为多个国内/苹果/备用探针，避免国内网络误判为 `Connected failed or no Internet`。
- WiFi Setup 提交配网时延长等待连接时间，并在等待期间保持 AP/DNS captive portal 服务，减少手机弹窗页面断开。
- WiFi Setup 成功保存时显示设备 STA IP；失败时区分“WiFi 未连接”和“已接入但外网检测失败”。

## 2026-05-28 01:34 +08:00

- 恢复 Clapping Music 为 `PRODUCT_GUIDE.md` 描述的 12 步节奏游戏结构：Easy/Medium/Hard、Play/Practice、M5/YOU 双行节拍点、ACC 评分、Advance 进度、暂停/继续和结果页。
- Clapping Music 与 WiFi Setup 程序页不再显示功能选择页大图，改为程序自己的操作/状态页面。
- Woodfish、RPS、Coin Flip 的摇动触发改为进入程序后延迟启用，并使用加速度变化量判断，避免一进入程序就误触发。
- RPS 结果页新增 `Rock`、`Scissors`、`Paper` 名称显示。
- WiFi Setup 页面新增热点密码、访问地址、STA 连接状态和本机 IP 显示。
- Haiyan 改为粉蓝水粒子并局部擦除/重绘，减少整屏黑闪；重力响应和边界碰撞参数重新调校。

## 2026-05-28 00:40 +08:00

- 按 UIFlow2 版 README 对齐 native C++ 主页面：恢复北京时间日期/星期/时间、电量百分比、电量条、A/B 进入菜单提示和底部署名。
- 对齐功能选择页交互：A 进入功能，B 短按切换功能，B 长按返回 Home；独立功能内 B 长按返回当前功能卡片。
- Clapping Music 改为难度/Play/Practice 菜单、游戏计数、暂停/继续和练习模式。
- Voice Memo 改为 `+ New rec` / 多条 `Memo` 列表式入口，支持最多 8 个 native WAV 槽位、录音、保存、播放和长按 A 删除当前录音，并兼容旧 `/memo.wav`。
- RPS 改为触发后快速闪烁结果图片，再居中显示最终石头/剪刀/布图片，并去掉结果页 `RPS` 标题。
- Sandtimer 改为可启动/切换预设/切换主题/长按取消的动态沙漏页面。
- WiFi Setup 保存逻辑改为联网并通过外网检查后才写入配置，同时成功联网后同步 RTC。
- README 同步更新为当前 native C++ 的真实功能状态，避免继续引用旧的“基础入口”描述。

## 2026-05-27 23:41 +08:00

- 新建并整理 Arduino/PlatformIO 原生 C++ 工程 `D:\Codex\M5StickS3PersonalConsole`。
- 接入本地 `M5Unified`、`M5GFX`、`ArduinoJson` 依赖。
- 配置 LittleFS 资源打包，确认所有功能页图片、RPS 结果图片、木鱼帧图和木鱼 WAV 音效都进入文件系统镜像。
- 修复木鱼 WAV 播放内存生命周期，避免后台播放时缓冲区提前释放。
- 功能选择页统一使用 `135x135` 图片资源，并在图片下方显示功能名称。
- RPS 程序结果改为加载 `rps_rock.jpg`、`rps_scissors.jpg`、`rps_paper.jpg`。
- WiFi Setup 增加 AP + DNS captive portal + 网页保存 WiFi；离开功能时关闭 portal。
- Weather 增加 IP 粗定位和 Open-Meteo 7 天天气/温度范围显示。
- 编译验证通过，LittleFS 镜像生成通过。

