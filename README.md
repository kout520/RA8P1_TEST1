# RA8P1_TEST1 · 智能取物柜 / 考勤门禁系统
> 基于 **Renesas RA8P1（Cortex-M85 + Helium DSP）** 的多因素身份认证取物柜系统，集 **NFC 刷卡、指纹、声纹 + 语音指令、串口屏交互、云端考勤上报、语音播报** 于一体，全流程片内裸机运行！
[![RA8P1](https://img.shields.io/badge/MCU-RA8P1%20(Cortex--M85)-blue.svg)](https://www.renesas.com/ra8p1)
[![e2studio](https://img.shields.io/badge/IDE-e%C2%B2%20studio%20%2F%20FSP-green.svg)](https://www.renesas.com/e2studio)
[![Bare-metal](https://img.shields.io/badge/RTOS-Bare--metal-orange.svg)]()
[![Voice AI](https://img.shields.io/badge/Voice-AI%20On--Device-red.svg)]()

---
## 📖 项目简介
本项目是基于 **Renesas RA8P1**（Cortex-M85 内核，主频 1GHz，内置 Helium DSP/MVE 加速单元）开发的**智能取物柜 + 考勤门禁系统**，采用 **e² studio / FSP** 开发，全裸机（无 RTOS）运行。

系统支持 **NFC 刷卡（PN532）、指纹识别（AS608）、声纹验证 + 语音指令（INMP441 麦克风）** 三种身份认证方式，内置**片内语音识别引擎**（唤醒词「大白」+ MFCC 特征提取 + DTW 指令匹配 + 声纹 Speaker Verification），无需云端算力即可离线完成语音鉴权与指令分发。识别结果通过 **TJC 串口屏** 实时展示，经 **天问语音模块** 语音播报，并通过 **ESP32 网络模块** 上传云端实现考勤上报、时间同步与库存管理。整套系统具备**打卡考勤、积分兑换取物、柜门开关控制、库存/余额提醒**等完整业务闭环。✨

---
## ✨ 核心特性
- 🧠 **RA8P1 高性能主控**：Cortex-M85 @1GHz + Helium DSP，片内完成 MFCC/DTW/声纹全链路 AI，无外部算力芯片
- 🎤 **片内语音识别引擎**：唤醒词「大白」→ 指令识别（打卡/可乐/雪碧/宏宝莱/牛奶/信息），自适应 VAD + 施密特滞回降误触发
- 🔊 **声纹验证 Speaker Verify**：录入/识别/断电恢复（经 ESP32 存 speaker.txt），陌生声音硬拒 + 灰区安全网
- 💳 **NFC 刷卡**：PN532 读取 MIFARE/NTAG 卡 UID，授权卡库比对，双向防抖
- 👆 **指纹识别**：AS608 全指令协议（录入/搜索/删除/清库），串口屏图形化管理
- 📺 **TJC 串口屏 GUI**：识别结果、指纹/NFC 管理、五项识别参数在线调节（VAD/TH_BASE/DTW_MARGIN/WAKE_THRESHOLD/SPK_SIM_THRESHOLD）、WiFi 配置
- ☁️ **云端联动**：ESP32 MQTT 考勤上报、NTP 时间同步、软件 RTC 断网续走
- 📢 **语音播报**：天问语音模块，打卡+积分、取物-积分、库存/余额不足、柜门开关等全程语音提示
- 🗄️ **柜门控制**：指令触发开柜门，70s 无操作自动关闭并取消认证
- 📷 **扩展外设**：OV5640 摄像头（CEU 采集）+ ILI9341 LCD（软件 SPI）辅助显示/头像，SD 卡存储，SDRAM 大容量缓冲

---
## 🧩 已集成模块清单
### 🎤 语音识别与声纹引擎
| 模块名称 | 功能说明 |
| :--- | :--- |
| inmp441 | INMP441 MEMS 麦克风驱动，I2S（SSI1）+ DTC DMA 双缓冲 16kHz 采集 |
| mfcc_engine | 片内 MFCC 特征提取引擎 |
| dct_matrix / hann_window / mel_filterbank | MFCC 数学库（DCT 矩阵 / 汉宁窗 / Mel 滤波器组） |
| voice_cmd | 指令识别（DTW 匹配：打卡/可乐/雪碧/宏宝莱/牛奶/信息） |
| wake_word / templates_dtw / model_*.h | 唤醒词「大白」与 DTW 指令模板库 |
| speaker_verify | 声纹 Speaker Verification（录入/识别/导出） |
| mram_kv | NVRAM 键值存储（声纹模型掉电保存） |
| voice | 天问语音模块驱动（UART1，考勤/取物/柜门语音播报） |

### 🔐 身份认证模块
| 模块名称 | 功能说明 | 接口 |
| :--- | :--- | :--- |
| pn532 | PN532 NFC 驱动，MIFARE/NTAG 读卡，授权卡库访问控制 | I2C2 |
| AS608pro | AS608 指纹驱动，全指令协议 + 交互式录入/搜索状态机 | UART2 |

### 🖥️ 人机交互模块
| 模块名称 | 功能说明 | 接口 |
| :--- | :--- | :--- |
| tjc_usart_hmi | TJC 串口屏驱动，命令帧解析、指纹/NFC/参数管理、WiFi 配网 | UART6 |
| lcd / text / font / face_avatar | ILI9341 LCD（软件 SPI），中英文显示、头像/待机界面 | GPIO |
| key | 按键模块 | GPIO |

### 🌐 网络与云端
| 模块名称 | 功能说明 | 接口 |
| :--- | :--- | :--- |
| esp32_comm | ESP32 通信驱动，MQTT 考勤上报、NTP 时间同步、软件 RTC、声纹下发 | UART3 |

### 📷 感知与存储
| 模块名称 | 功能说明 | 接口 |
| :--- | :--- | :--- |
| ov5640 | OV5640 摄像头驱动（分辨率/格式/白平衡/自动对焦） | CEU + I2C0 |
| SD 卡 / SDRAM | SDHI/MMC 存储、外部总线 SDRAM 帧缓冲与录音缓冲 | SDHI/BUS |

---
## 🚀 快速上手
### 1. 环境要求
- **主控芯片**：Renesas RA8P1（Cortex-M85，1GHz）
- **开发工具**：Renesas e² studio + FSP（工程基于 FSP 配置器生成）
- **调试器**：J-Link（工程含 `RA8P1_TEST1 Debug_Flat.jlink` / `.launch`）
- **外设**：INMP441 麦克风、TJC 串口屏、AS608 指纹、PN532 NFC、ESP32 网络模块、天问语音模块、OV5640 摄像头、ILI9341 LCD、SD 卡、SDRAM

### 2. 编译与烧录
1.  克隆本仓库到本地：
    ```bash
    git clone https://github.com/kout520/RA8P1_TEST1.git
    ```
2.  用 **e² studio** 打开工程（`.cproject` / `.project`），FSP 配置见 `ra_cfg/`、`ra_gen/`
3.  连接 J-Link，选择 `RA8P1_TEST1 Debug_Flat` 调试配置，编译烧录运行
4.  `src/hal_entry.c` 为应用入口，业务模块集中在 `code/`，FSP 生成代码在 `ra_gen/`（请勿手改）

### 3. 串口调试命令（UART9，printf 重定向）
| 命令 | 功能 |
| :--- | :--- |
| `r` | 切到识别模式（默认） |
| `c` | 切到模板采集模式 |
| `k` | 开始录音（采集 2s / 声纹录入 10s） |
| `e` | 声纹录入模式（`f` 完成并导出给 ESP32） |
| `f` | 完成声纹录入 |
| `g` | 清空全部声纹（并通知 ESP32 删除 speaker.txt） |

### 4. 使用流程示例
```bash
# ① 声纹录入（首次使用）
按 e 进入录入 → 按 k 录音 10s（"我是小白"）→ 按 f 完成
→ 声纹自动导出到 ESP32（断电后自动恢复）

# ② 日常使用
说「大白」唤醒 → 说「打卡」→ 声纹验证通过 → 指纹/NFC 打卡 → 串口屏显示 + 语音播报
说「可乐」→ 验证通过 → 积分-5 → 开柜门 → 语音提示取物
```

---
## 📂 项目结构
```
RA8P1_TEST1/
├── src/                     # 应用入口
│   ├── hal_entry.c          # 主程序：语音识别主循环 + 任务调度（约 36KB）
│   └── hal_warmstart.c      # BSP 启动钩子（IOPORT / SDRAM 初始化）
├── code/                    # 业务模块（语音/声纹/NFC/指纹/串口屏/网络/摄像头等）
│   ├── inmp441.c / mfcc_engine.c / voice_cmd.c / speaker_verify.c   # 语音识别与声纹
│   ├── dct_matrix.h / hann_window.h / mel_filterbank.h              # MFCC 数学库
│   ├── wake_word.h / templates_dtw.h / model_*.h                    # 唤醒词与 DTW 模板
│   ├── pn532.c / AS608pro.c                                         # NFC / 指纹
│   ├── tjc_usart_hmi.c / lcd.c / text.c / font.c / face_avatar.h    # 人机交互
│   ├── esp32_comm.c / voice.c / command_dispatch.c / mram_kv.c      # 网络/播报/调度/存储
│   └── ov5640.c                                                      # 摄像头
├── ra_gen/                  # FSP 自动生成代码（勿手改）
├── ra_cfg/  ra/             # FSP 配置
├── script/                  # 链接脚本（fsp.ld）
├── Debug/  tools/           # 构建与工具
├── *.py                     # PC 端训练/分析工具（模板生成、唤醒词、交叉/能量分析）
├── *.pkl                    # 离线模型数据
├── PROGRAM_MANIFEST.md      # 完整程序清单（源码/外设/引脚/时钟/中断）
├── PIN_WIRING_DIAGRAM.md    # 引脚接线图
├── 流程图.md                # 语音交互系统完整流程图
├── 优化.md                  # 指纹/NFC/参数调节开发记录
└── .gitignore
```

---
## 🛠️ PC 端工具（语音训练与调参）
| 脚本 | 功能 |
| :--- | :--- |
| `gen_templates.py` | 生成 DTW 指令模板 |
| `gen_wake.py` | 生成唤醒词模板 |
| `analyze_cross.py` / `analyze_templates.py` | 指令模板交叉/质量分析 |
| `analyze_c_energy.py` / `analyze_wake.py` | 指令能量 / 唤醒词分析 |
| `model_quick.pkl` | 离线模型数据 |

---
## 📄 开源协议
本项目当前**未指定开源许可证**。如需对外开源，请自行添加 `LICENSE` 文件（FSP 生成代码受 Renesas FSP License 约束，请留意）。

---
## 🎉 致谢
感谢 Renesas 提供的 RA8P1 高性能 MCU 与 FSP 开发框架，感谢各位嵌入式开发者的交流与贡献！
---
