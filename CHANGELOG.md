# 更新日志

### 2026-05-29 继续更新
- 新增最后一个隐藏名称的 `Loveletter` 信箱程序：Day1 收到鼓槌、Day6575 收到麦克风、Day8785 收到海盐们的爱，最后显示中文寄语。入口只显示信箱图形，不显示功能名称。
- 调整 `Loveletter` 的 Day8785 流程：先显示 Day8785 关闭信箱页，再按 A 打开信箱并弹出“收到了海盐们的爱”。
- 保存当前说明书文字稿到 `docs/manual_draft.md`，后续 A6 说明书排版可以继续基于该底稿修改。
- 功能选择栏顺序调整：`Dino` 移到 `Clapping Music` 和 `Voice Memo` 中间；`Volume` 放到 `WiFi Setup` 前面；`Loveletter` 放在最后。仅调整入口顺序，不改变已有功能内部行为。
- 新增 `Volume` 音量调节程序：通过类似木鱼的纵向敲击手势逐格调节系统音量，并用柱状条显示当前占比；调节值保存到 NVS，木鱼、提示音和录音播放共用该系统音量。
- Coin Flip、RPS、Woodfish 的动作触发统一为完整纵向敲击一次才触发，普通摇晃不触发；Sandtimer 和 Haiyan 继续使用各自的重力方向逻辑。
- Voice Memo 录音保存与播放链路修复：每段录音独立保存到 `/memo_0.wav` 到 `/memo_7.wav`，删除后 LittleFS 释放对应文件；播放已保存录音时显示 `Playing`。
- 修复连续录制多段后旧录音播放无声的问题：播放录音前关闭 Mic 并重启 Speaker，录音停止后释放 Mic，避免录音和扬声器共用音频通道导致后续播放静音。
- 修复 Woodfish 在 Voice Memo 使用后无声的问题：木鱼 WAV 播放前同样切回 Speaker 音频状态，不再被 Mic 占用影响。

### 2026-05-29 09:32 +0800
- WiFi Setup 保存逻辑从单个 `ssid/pass` 扩展为最多 5 个已保存网络，并兼容迁移旧的单网络配置。
- WiFi Setup 设备页新增已保存网络序号显示：Button B 短按切换保存项，Button A 短按连接当前保存项，长按 Button A 删除当前保存项。
- WiFi 自动连接会优先尝试当前选中的保存项，并在失败后继续轮询其它已保存网络；网页配网成功会新增或更新保存列表。
- 新增 `Today History` 历史上的今天功能：按当前 RTC 日期请求 Wikimedia On this day 事件，Button A 刷新，Button B 切换事件，网络失败时显示离线提示和少量兜底事件。
- 新增功能页资源 `data/img/history.jpg`。
- 精简固件体积：移除未使用的 PNG 图片显示路径、旧木鱼 PNG 资源和未读取的 Weather/History 状态变量，当前资源统一使用 JPG 帧。
- Today History 进入功能后会自动加载一次当天事件，减少必须先按 A 的空白感。
- Haiyan 改为三轴 IMU 重力映射，增加粒子数量、低粘度流动和轻微凝聚力，让水体晃动更贴近容器里的流体。
- Clapping Music 放宽 Easy/Medium/Hard 节拍判定窗口，降低通过门槛，并强化当前拍、目标拍和用户输入的显示层级。
- Home 主时钟页面重排为大号时间卡片、日期、城市、WiFi 状态和电量条。
- Sandtimer 重做为动态沙漏界面，运行时显示落沙、剩余时间和进度条，并保留 A/B/长按 A 操作。
- 所有程序重新进入时重置到各自初始页面；Coin Flip、RPS、Woodfish 的摇动触发保留。
- Haiyan 重力方向改为参考 bottle-of-ocean 的 `-ax/-ay` 轴映射，让设备换姿态后海水流向新的瓶底。
- Clapping Music 难度页底部提示改为 `A start   B select`，所有黄色节拍/选中/分数显示改为粉色，游戏中按下 A 的瞬间就记录输入。
- Home 去掉大块绿色电池条，改为右上角百分比加小电池图标；时间继续放大，并新增 `data/img/yr_logo.jpg` 与深红色 `Don't be afraid`。
- Sandtimer 改为重力左右晃动选择 0-60 分钟，每次 30 秒；A 确认倒计时，倒计时页蓝色上沙、粉色下沙，A 暂停，B 切换主题，暂停后长按 B 回时间选择。
- 修正 Haiyan 对角线姿态下的瓶底方向：IMU X/Y 对屏幕轴交换后再映射重力，解决左下/右上瓶底显示相反的问题。
- Home 恢复时间上方电池条，去掉主页面右上角电池图标和秒数显示；日期居中，主页面只显示 WiFi 状态不显示位置，并补上 `Designated by YIlin`。
- Clapping Music 的蓝色和粉色调为更浅嫩的色值；程序选择页状态栏电池图标左移，避免被百分比遮挡；Sandtimer 时间选择页改为横向仪表式显示，强调左右晃动选择时长。
- 颜色基准调整为 PMS 1767 C 近似粉色与 PMS 297 近似蓝色，黄色显示尽量收敛为 PMS 1767。
- Sandtimer 选择时长改用屏幕横向重力轴，只有横向倾斜明显大于纵向时才增减，纵向晃动不改变时间；最低保持 `00:00`，再向右从 `00:00` 开始增加。
- Today History 优先筛选音乐史/摇滚相关条目，并从当天事件、出生、去世接口中补充音乐人内容，找不到音乐相关时再退回普通历史事件。
- Sandtimer 进入时默认时长改为 30 秒。
- Today History 网络/API 都失败时把状态从 `HTTP fail` 改为 `Offline/API`，避免看起来像具体 HTTP 协议错误。
- 从主时钟页按 A/B 再进入程序选择页时，始终回到第一个程序卡片。
- Dino 游戏中按住 B 趴下的最长持续时间限制为约 1.8 秒，避免长按一直保持趴下。
- Today History 在 Wikimedia/Wikipedia API 被网络环境阻断时优先显示本地音乐史兜底；5/29 改为 Noel Gallagher/Oasis 等音乐相关事件，不再退回普通登山事件。

### 2026-05-28 18:42 +0800
- Haiyan 背景改为按北京时间自动切换高透明度天空色：晨间红蓝、日间蓝绿/蓝色、傍晚蓝粉、夜间深蓝或黑色，并继续保持粉蓝半透明水体显示。
- Haiyan 重力响应、阻尼、边界反弹和粒子分离参数继续向低粘度实时流体方向调整，倾斜响应更快。
- 新增 `Dino` 小恐龙游戏：A 跳跃，B 趴下，游戏中长按 B 暂停，暂停/入口/结束页长按 B 返回功能选择页。
- Dino 结束页支持 A 重新开始、B 返回入口页，最高分保存到 NVS，重启后保留。
- 新增功能页资源 `data/img/dino.jpg`。

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

