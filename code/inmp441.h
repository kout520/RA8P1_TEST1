/**
 * INMP441 MEMS 麦克风 - 头文件
 *
 * 硬件: INMP441 I2S 数字麦克风
 * 接口: SSI1 (I2S Master 模式) + DTC (DMA RX)
 *
 * 接线:
 *   INMP441 SCK  → P206 (SSI1_SCK)
 *   INMP441 WS   → P906 (SSI1_WS)
 *   INMP441 SD   → P907 (SSI1_RXD)
 *   INMP441 L/R  → GND (左声道)
 *   INMP441 VDD  → 3.3V
 *   INMP441 GND  → GND
 *
 * 依赖 FSP 配置:
 *   - g_i2s1:  SSI1, Master, 32-bit PCM, 32-bit word, DTC RX 使能
 *   - g_timer1: GPT1 提供音频时钟 (需根据采样率调整 period 值)
 *   - g_transfer_rx: DTC 块传输, 触发源 SSI1_RXI
 *
 * 推荐采样率: 16 kHz
 *   GPT1 period ≈ 234 (for PCLKD=240MHz, bit_clock=1.024MHz)
 */
#ifndef __INMP441_H__
#define __INMP441_H__

#include <stdint.h>
#include "hal_data.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ===== 音频缓冲区 ===== */

/** 单次 DMA 缓冲区: 4096 字节 = 1024 个32-bit字 = 512 个立体声帧
 *  INMP441 挂在左声道, 每个缓冲区 512 个有效采样点
 *  16kHz 下约 32ms 延迟 */
#define INMP441_BUF_SIZE    4096

/** 采样点个数 (单声道, 16-bit) */
#define INMP441_SAMPLES_PER_BUF   (INMP441_BUF_SIZE / 8)   /* 4096/(4×2) = 512 */

/* ===== 公共接口 ===== */

/**
 * 初始化 INMP441 音频采集
 * 1. 打开 g_timer1 (音频时钟源)
 * 2. 打开 g_i2s1 (SSI1 I2S 接口)
 * 3. 启动第一次 DMA 接收
 *
 * @return FSP_SUCCESS 成功, 否则失败
 */
fsp_err_t inmp441_init(void);

/**
 * 获取最新完成的音频缓冲区 (16-bit 单声道)
 * 数据由 DMA 自动填充, 回调中切换双缓冲
 *
 * @return 指向最新缓冲区的指针, 未就绪时返回 NULL
 *         数据长度 = INMP441_SAMPLES_PER_BUF 个 int16_t
 *
 * 注意: 每次调用返回的数据在下一次调用前有效
 *       需要连续读取时, 调用者应自行拷贝数据
 */
int16_t * inmp441_get_buffer(void);

/**
 * 获取最近完成的原始 32-bit I2S DMA 缓冲区（调试用）
 * @return 指向原始缓冲区的指针 (uint32_t*), 没新数据时返回 NULL
 *         长度 = INMP441_BUF_SIZE / 4 个 32-bit 字
 */
uint32_t * inmp441_get_raw_buffer(void);

/**
 * 获取音频峰值电平 (最近一个缓冲区的绝对值峰值)
 *
 * @return 0 ~ 32767 的峰值, 无数据时返回 0
 */
int32_t inmp441_get_peak(void);

/**
 * 停止音频采集 (关闭 SSI1, 停止 DMA)
 *
 * @return FSP_SUCCESS 成功, 否则失败
 */
fsp_err_t inmp441_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* __INMP441_H__ */
