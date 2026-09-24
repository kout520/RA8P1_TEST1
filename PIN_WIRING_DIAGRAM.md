# RA8P1 外设引脚接线图

## 1. SSI1 — INMP441 I2S 麦克风

| RA8P1 引脚 | FSP 功能 | 连接设备 | INMP441 引脚 | 备注 |
|-----------|---------|---------|-------------|------|
| **P206** | SSIE1 DATA (ssidata1) | INMP441 | **SD** (数据输出) | 最初接错成 SCK，已修正 |
| **P906** | SSIE1 LRCK (ssilrck1) | INMP441 | **WS** (字选) | |
| **P907** | SSIE1 BCK (ssibck1) | INMP441 | **SCK** (位时钟) | 最初接错成 SD，已修正 |
| **P402** | SSIE AUDIO_CLK | **飞线 → P105** | — | 外部音频时钟输入 |
| — | — | INMP441 | **L/R** → GND | 左声道 |
| — | — | INMP441 | **VDD** → 3.3V | |
| — | — | INMP441 | **GND** → GND | |

> ⚠ **关键飞线**: P402 (AUDIO_CLK) ←→ P105 (GPT1 GTIOCA)
> 内部路由不通，需外部飞线提供 SSI 过采样时钟

---

## 2. GPT 定时器

| RA8P1 引脚 | FSP 功能 | 用途 | 备注 |
|-----------|---------|------|------|
| P713 | GPT2 GTIOCA (gtioc2a) | GPT PWM 输出 | |
| P102 | GPT2 GTIOCB (gtioc2b) | GPT PWM 输出 | |
| **P105** | GPT1 GTIOCA (gtioc1a) | **SSI1 音频时钟源** | **飞线到 P402** |
| P104 | GPT1 GTIOCB (gtioc1b) | GPT PWM 输出 | |
| — | GPT0 (内部) | 1ms 系统定时器 | 无外部引脚 |

| FSP 实例 | GPT 单元 | 通道 | 模式 | 用途 |
|---------|---------|------|------|------|
| g_timer_1ms | GPT0 | 0 | Periodic (1ms) | 系统滴答 |
| g_timer1 | GPT1 | 1 | Periodic (234 counts) | SSI1 音频时钟 (~1.07MHz) |
| g_timer4 | ? | 4 | PWM | 通用 PWM |

---

## 3. UART 串口

| FSP 实例 | SCI 通道 | RA8P1 引脚 | FSP 功能 | 连接设备 | 波特率 |
|---------|---------|-----------|---------|---------|--------|
| **g_uart9** | SCI9 | **P208** | RXD9 | 调试串口 RX (PC) | 115200 |
| | | **P209** | TXD9 | 调试串口 TX (PC) | 115200 |
| **g_uart1** | SCI1 | **P706** | RXD1 | 天问语音模块 RX | 115200 |
| | | **P707** | TXD1 | 天问语音模块 TX | 115200 |
| **g_uart2** | SCI2 | **P801** | TXD2 | AS608 指纹  | 57600 |
| | | **P802** | RXD2 | AS608 指纹  | 57600 |
| **g_uart3** | SCI3 | **P310** | TXD3 |  ESP32 TX | 115200 |
| | | **P905** | RXD3 |   ESP32 RX| 115200 |
| **g_uart6** | SCI6 | **P908** | TXD6 | TJC 串口屏 TX | 115200 |
| | | **P909** | RXD6 | TJC 串口屏 RX | 115200 |

---

## 4. I2C 总线

| FSP 实例 | IIC 通道 | RA8P1 引脚 | FSP 功能 | 连接设备 | 速率 |
|---------|---------|-----------|---------|---------|------|
| **g_i2c_master0** | IIC0 | **P409** | SDA0 | OV5640 摄像头 SCCB | 393 kHz |
| | | **P410** | SCL0 | OV5640 摄像头 SCCB | 393 kHz |
| **g_i2c2** | IIC2 | **P514** | SDA2 | PN532 NFC 模块 | 97 kHz |
| | | **P515** | SCL2 | PN532 NFC 模块 | 97 kHz |

---

## 5. CEU — OV5640 摄像头（8-bit 并行）

| RA8P1 引脚 | FSP 功能 | OV5640 信号 | 说明 |
|-----------|---------|------------|------|
| **P400** | CEU D0 (vio_d0) | D0 | 数据位 0 |
| **P401** | CEU D1 (vio_d1) | D1 | 数据位 1 |
| **P405** | CEU D2 (vio_d2) | D2 | 数据位 2 |
| **P406** | CEU D3 (vio_d3) | D3 | 数据位 3 |
| **P700** | CEU D4 (vio_d4) | D4 | 数据位 4 |
| **P701** | CEU D5 (vio_d5) | D5 | 数据位 5 |
| **P702** | CEU D6 (vio_d6) | D6 | 数据位 6 |
| **P703** | CEU D7 (vio_d7) | D7 | 数据位 7 |
| **P414** | CEU CLK (vio_clk) | PCLK | 像素时钟 |
| **P415** | CEU HD (vio_hd) | HREF | 行同步 |
| **P708** | CEU VD (vio_vd) | VSYNC | 帧同步 |
| P409 | IIC0 SDA | SCCB SDA | 摄像头配置 |
| P410 | IIC0 SCL | SCCB SCL | 摄像头配置 |

---

## 6. GPIO 控制引脚

| RA8P1 引脚 | 方向 | 初始电平 | 推测用途 |
|-----------|------|---------|---------|
| **P110** | OUT | LOW | LCD 相关 (SPI CS?) |
| **P601** | OUT | LOW | LCD CS (片选) |
| **P602** | OUT | LOW | LCD RST (复位) |
| **P603** | OUT | HIGH | LCD RS (数据/命令) |
| **P604** | OUT | HIGH | LCD BL (背光) |
| **P605** | OUT | LOW | 预留 / LCD SCK? |
| **P711** | OUT | LOW|继电器
| **P807** | IN | — | AS608 指纹触摸检测 (PS_Sta) |

---

## 7. SDHI/MMC — SD 卡

| RA8P1 引脚 | FSP 功能 |
|-----------|---------|
| P111 | SDHI0 DAT3 |
| P1301 | SDHI DAT? |
| P1302 | SDHI DAT? |
| P1303 | SDHI DAT? |
| P1304 | SDHI DAT? |
| P1305 | SDHI DAT? |
| P1307 | SDHI DAT? |

---

## 8. BUS — 外部总线 (SDRAM/LCD 并行)

| 端口 | 引脚 | 用途 |
|------|------|------|
| Port 1 | P112-P115 | 外部总线 |
| Port 3 | P300-P302 | 外部总线 |
| Port 5 | P503-P510 | 外部总线 |
| Port 6 | P607-P615 | 外部总线 |
| Port 8 | P813 | 外部总线 |
| Port 10 | P1000-P1015 | 外部总线 |
| Port 12 | P1200-P1215 | 外部总线 |
| Port 13 | P1300 | 外部总线 |

---

## 9. DEBUG — 调试接口

| RA8P1 引脚 | 功能 |
|-----------|------|
| P210 | SWDIO |
| P211 | SWCLK |

---

## 10. 中断向量和 ISR

| 向量号 | 中断 | ISR 函数 | 优先级 | 外设 |
|--------|------|---------|--------|------|
| 0 | SCI9 RXI | sci_b_uart_rxi_isr | 12 | 调试串口 |
| 1 | SCI9 TXI | sci_b_uart_txi_isr | 12 | |
| 2 | SCI9 TEI | sci_b_uart_tei_isr | 12 | |
| 3 | SCI9 ERI | sci_b_uart_eri_isr | 12 | |
| 4 | CEU CEUI | ceu_isr | 12 | OV5640 |
| 5-8 | IIC0 RXI/TXI/TEI/ERI | iic_master_*_isr | 12 | OV5640 SCCB |
| 9-12 | SCI6 RXI/TXI/TEI/ERI | sci_b_uart_*_isr | 12 | 预留 |
| 13-16 | IIC2 RXI/TXI/TEI/ERI | iic_master_*_isr | 12 | PN532 NFC |
| 17-20 | SCI1 RXI/TXI/TEI/ERI | sci_b_uart_*_isr | 12 | 语音模块 |
| 21-24 | SCI2 RXI/TXI/TEI/ERI | sci_b_uart_*_isr | 12 | AS608 指纹 |
| 25-28 | SCI3 RXI/TXI/TEI/ERI | sci_b_uart_*_isr | 12 | TJC 串口屏 |
| 29 | GPT0 OVERFLOW | gpt_counter_overflow_isr | 12 | 1ms 定时器 |
| 30 | **SSI1 RXI** | ssi_rxi_isr | **2** | INMP441 DTC |
| 31 | **SSI1 INT** | ssi_int_isr | **2** | INMP441 错误 |

---

## 系统框图

```
                         ┌──────────────────────────────────────┐
                         │           RA8P1 (R7KA8P1KF)          │
                         │                                      │
    ┌────────┐           │  P208/P209 ── UART9(SCI9) ── PC调试  │
    │ INMP441│ I2S       │  P206      ── SSI1 DATA ◄── SD      │
    │ 麦克风  │◄─────────│  P906      ── SSI1 LRCK ──► WS     │
    │        │           │  P907      ── SSI1 BCK  ──► SCK    │
    └────────┘           │  P402      ── AUDIO_CLK ◄──┐       │
                         │  P105      ── GPT1 GTIOCA ─┘ 飞线   │
    ┌────────┐           │                                      │
    │ OV5640 │ 8-bit CEU │  P400-P703 ── CEU D0-D7             │
    │ 摄像头  │◄─────────│  P414/P415 ── CEU CLK/HD            │
    │        │ I2C       │  P409/P410 ── IIC0 SDA/SCL          │
    └────────┘           │  P708      ── CEU VD                │
                         │                                      │
    ┌────────┐           │  P706/P707 ── UART1(SCI1) ── 语音   │
    │语音模块 │◄─────────│                                      │
    └────────┘           │  P801/P802 ── UART2(SCI2) ── AS608  │
    ┌────────┐           │                       ── ESP32      │
    │ AS608  │◄─────────│  P807      ── GPIO IN (触摸检测)    │
    │ 指纹   │           │                                      │
    └────────┘           │  P310/P905 ── UART3(SCI3) ── TJC屏  │
                         │                                      │
    ┌────────┐           │  P514/P515 ── IIC2 SDA/SCL          │
    │ PN532  │ I2C       │                                      │
    │ NFC    │◄─────────│                                      │
    └────────┘           │  P601-P605 ── GPIO ── ILI9341 LCD  │
                         │                                      │
    ┌────────┐           │  P111,13xx ── SDHI ── SD卡          │
    │ ILI9341│ SPI(GPIO) │                                      │
    │  LCD   │◄─────────│  Port5/6/10/12 ── BUS ── SDRAM     │
    └────────┘           │                                      │
                         └──────────────────────────────────────┘
```
