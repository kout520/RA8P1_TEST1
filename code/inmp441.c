/**
 * INMP441 MEMS 麦克风 - 实现文件
 *
 * 硬件: INMP441 通过 I2S 接口连接到 SSI1
 * 协议: I2S (MSB first, 1 BCLK delay from WS edge)
 * 数据: 24-bit signed, 挂载在左声道 (L/R 引脚接 GND)
 *
 * 双缓冲 DMA 采集:
 *   - DMA 自动将 SSI FIFO 数据搬运到 raw_buf[0]
 *   - 完成后回调切换到 raw_buf[1], 同时 raw_buf[0] 可被读取
 *   - 每个缓冲区 512 个采样点 (16kHz 下约 32ms)
 *
 * 依赖: FSP g_i2s1 (SSI1) + g_transfer_rx (DTC) + g_timer1 (GPT 音频时钟)
 */

#include "inmp441.h"
#include "headfile.h"

/* ==================================================================
 *                   BSP 内存对齐宏 (兼容不同编译器)
 * ================================================================== */
#ifndef BSP_ALIGN_VARIABLE
#define BSP_ALIGN_VARIABLE(n)  __attribute__((aligned(n)))
#endif

/* ==================================================================
 *                    内部变量 (双缓冲)
 * ================================================================== */

/** 双 DMA 原始缓冲区 (32 字节对齐, 用于 Cache 操作) */
static uint8_t  g_raw_buf[2][INMP441_BUF_SIZE] BSP_ALIGN_VARIABLE(32);

/** 转换后的 16-bit 单声道采样 (32 字节对齐) */
static int16_t  g_audio_samples[INMP441_SAMPLES_PER_BUF] BSP_ALIGN_VARIABLE(32);

/** g_buf_ready_idx:
 *    -1 = 没有就绪的缓冲区
 *    0/1 = 指示哪个 raw_buf 已填满, 可以读取 */
static volatile int g_buf_ready_idx = -1;

/** g_buf_active_idx: 当前 DMA 正在填充的 raw_buf 索引 */
static int g_buf_active_idx = 0;

/** 上一次缓冲区的峰值电平 (0 ~ 32767) */
static volatile int32_t g_audio_peak = 0;

/** 音频采集是否正在运行 */
static volatile bool g_capture_running = false;

/* ==================================================================
 *        DC 阻隔滤波器状态 (跨缓冲区连续, 消除块边界跳变)
 * ================================================================== */

/** 数字增益: INMP441 灵敏度低(-26dBFS@94dB), 6x=+15.6dB (8x 会让爆破音如"打卡"削波) */
#define INMP441_DIGITAL_GAIN  6

/** DC 阻隔滤波器系数 (一阶高通, fc≈13Hz@16.7kHz)
 *  y[n] = x[n] - x[n-1] + R * y[n-1]
 *  R = 0.995 → Q15 定点: 0.995 × 32768 = 32604 */
#define DC_FILTER_R_Q15   32604

static int32_t dc_prev_x = 0;   /**< 上一个输入采样 x[n-1] */
static int32_t dc_prev_y = 0;   /**< 上一个输出采样 y[n-1] */

/* ==================================================================
 *                    前向声明
 * ================================================================== */
static void inmp441_start_read(int buf_idx);

/* ==================================================================
 *                   I2S 回调 (由 SSI1 中断触发)
 * ================================================================== */

/**
 * SSI1 I2S 回调函数
 *
 * FSP 在以下情况调用此回调:
 *   I2S_EVENT_RX_FULL  - DMA 完成一块数据的接收
 *   I2S_EVENT_IDLE     - SSI 进入空闲状态
 *   I2S_EVENT_TX_EMPTY - 发送缓冲区空 (仅 TX 模式, 不使用)
 */
void i2s1_callback(i2s_callback_args_t *p_args)
{
    if (NULL == p_args) return;

    switch (p_args->event)
    {
    case I2S_EVENT_RX_FULL:
    {
        /* 当前 DMA 缓冲区已填满, 标记为就绪 */
        int completed_idx = g_buf_active_idx;

        /* 使能 D-Cache 失效, 确保 CPU 读到 DMA 写入的最新数据 */
        SCB_InvalidateDCache_by_Addr((void *)g_raw_buf[completed_idx], INMP441_BUF_SIZE);

        /* 标记此缓冲区给应用层读取 */
        g_buf_ready_idx = completed_idx;

        /* 切换到另一个缓冲区, 启动下一次 DMA */
        g_buf_active_idx ^= 1;
        inmp441_start_read(g_buf_active_idx);
        break;
    }

    case I2S_EVENT_IDLE:
        /* SSI 已停止, 可以安全关闭 */
        g_capture_running = false;
        break;

    default:
        break;
    }
}

/* ==================================================================
 *                    内部函数
 * ================================================================== */

/**
 * 启动一次 DMA 接收, 将 SSI 数据搬运到指定缓冲区
 */
static void inmp441_start_read(int buf_idx)
{
    fsp_err_t err;

    err = g_i2s1.p_api->read(g_i2s1.p_ctrl,
                             g_raw_buf[buf_idx],
                             INMP441_BUF_SIZE);
    if (FSP_SUCCESS != err)
    {
        printf("INMP441: read start failed! err=%d\r\n", err);
    }
}

/**
 * 将原始 32-bit I2S 数据转换为 16-bit 单声道采样 (带 DC 阻隔 + 数字增益)
 *
 * 输入格式:
 *   - 32-bit 字: bits[31:8] = 24-bit signed PCM, bits[7:0] = 0
 *   - 偶数索引 = 左声道 (INMP441 数据), 奇数索引 = 右声道 (全零)
 *
 * 输出格式:
 *   - 16-bit signed PCM, 保留 24-bit 的高 16 位
 *   - 单声道, 仅取左声道
 *
 * DC 阻隔: 一阶高通 IIR 滤波器, 状态跨块保持, 无块边界跳变
 *   传递函数: H(z) = (1 - z⁻¹) / (1 - R·z⁻¹), R=0.995
 *   截止频率 ≈ 13 Hz @ 16.7kHz
 */
static void convert_raw_to_samples(uint8_t *raw_buf, int16_t *samples, int32_t *peak_out)
{
    int32_t *p32 = (int32_t *)raw_buf;
    int32_t  peak = 0;

    for (int i = 0; i < INMP441_SAMPLES_PER_BUF; i++)
    {
        /* 左声道: 24-bit → 高16位 */
        int32_t x = (int32_t)(p32[i * 2] >> 16);

        /* DC 阻隔滤波器: y[n] = x[n] - x[n-1] + R * y[n-1] */
        int32_t filtered = x - dc_prev_x + ((dc_prev_y * DC_FILTER_R_Q15) >> 15);
        dc_prev_x = x;
        dc_prev_y = filtered;

        /* 数字增益 (补偿 INMP441 低灵敏度) */
        int32_t gained = filtered * INMP441_DIGITAL_GAIN;

        /* 硬限幅到 int16_t 范围 */
        if (gained > 32767)  gained = 32767;
        if (gained < -32768) gained = -32768;

        samples[i] = (int16_t)gained;

        int32_t abs_val = (gained < 0) ? -gained : gained;
        if (abs_val > peak) peak = abs_val;
    }

    *peak_out = peak;
}

/**
 * 处理完成缓冲区的数据 (回调中调用)
 */
static void process_completed_buffer(int buf_idx)
{
    convert_raw_to_samples(g_raw_buf[buf_idx], g_audio_samples, (int32_t *)&g_audio_peak);
}

/* ==================================================================
 *                      公共接口
 * ================================================================== */

/**
 * 初始化 INMP441 音频采集
 *
 * 步骤:
 *   1. 打开 g_timer1 提供 SSI1 音频时钟 (内部路由 GPT1 → SSI1 AUDIO_CLK)
 *   2. 打开 g_i2s1 (SSI1 I2S 接口) + DTC RX
 *   3. 启动第一次 DMA 接收
 *
 * @retval FSP_SUCCESS  初始化成功
 * @retval 其他          失败 (打印错误码)
 */
fsp_err_t inmp441_init(void)
{
    fsp_err_t err;

    /* --- 1. 打开音频时钟源 (GPT1 → SSI1 AUDIO_CLK) --- */
    err = g_timer1.p_api->open(g_timer1.p_ctrl, g_timer1.p_cfg);
    if (FSP_SUCCESS != err)
    {
        printf("INMP441: GPT1 (audio clock) open failed! err=%d\r\n", err);
        return err;
    }
    g_timer1.p_api->start(g_timer1.p_ctrl);

    /* 直接写 GTIOR 寄存器: GTIOA=PWM mode1 (3), OAE=1
       绕过 FSP GPT 驱动在 PERIODIC 模式下不写 GTIOR 的问题 */
    R_GPT1->GTIOR = 0x00000103;
    printf("INMP441: GPT1 audio clock started (GTIOR=0x%08lX)\r\n",
           R_GPT1->GTIOR);

    /* --- 2. 打开 SSI1 I2S 接口 (自动配置 DTC) --- */
    err = g_i2s1.p_api->open(g_i2s1.p_ctrl, g_i2s1.p_cfg);
    if (FSP_SUCCESS != err)
    {
        printf("INMP441: SSI1 open failed! err=%d\r\n", err);
        return err;
    }
    printf("INMP441: SSI1 I2S opened\r\n");

    /* 强制 SSI 使用外部时钟 (CKS=0), 绕过 FSP 生成问题 */
    R_SSI1->SSICR &= ~R_SSI0_SSICR_CKS_Msk;
    printf("INMP441: CKS forced to EXTERNAL (SSICR=0x%08lX)\r\n",
           R_SSI1->SSICR);

    /* --- 3. 双缓冲初始化 --- */
    g_buf_active_idx = 0;
    g_buf_ready_idx  = -1;
    g_audio_peak     = 0;
    g_capture_running = true;

    /* 清空缓冲区 */
    memset(g_raw_buf, 0, sizeof(g_raw_buf));
    memset(g_audio_samples, 0, sizeof(g_audio_samples));

    /* 复位 DC 阻隔滤波器状态 */
    dc_prev_x = 0;
    dc_prev_y = 0;

    /* --- 4. 启动第一次 DMA 接收 --- */
    inmp441_start_read(0);

    printf("INMP441: init done, capture started (buf=%d samples)\r\n",
           INMP441_SAMPLES_PER_BUF);
    return FSP_SUCCESS;
}

/**
 * 获取最新完成的 16-bit 单声道音频数据
 *
 * 使用方式:
 *   int16_t *buf = inmp441_get_buffer();
 *   if (buf != NULL) {
 *       // 处理 INMP441_SAMPLES_PER_BUF 个采样点
 *       for (int i = 0; i < INMP441_SAMPLES_PER_BUF; i++) {
 *           int16_t sample = buf[i];
 *           // ...
 *       }
 *   }
 *
 * @return 指向音频数据的指针, 没有新数据时返回 NULL
 *         指针指向内部静态缓冲区, 下次调用时内容会被覆盖
 */
int16_t * inmp441_get_buffer(void)
{
    static int no_data_cnt = 0;

    int ready = g_buf_ready_idx;
    if (ready < 0) {
        /* 连续无数据 → DMA 链可能断裂, 自动重启 */
        no_data_cnt++;
        if (no_data_cnt >= 100) {  /* 约 3 秒无音频数据 */
            printf("INMP441: DMA stalled, restarting capture...\r\n");
            inmp441_start_read(g_buf_active_idx);
            no_data_cnt = 0;
        }
        return NULL;
    }

    no_data_cnt = 0;

    /* 将原始数据转换为 16-bit 单声道采样 */
    process_completed_buffer(ready);

    /* 标记已读取, 缓冲区归还给 DMA 循环 */
    g_buf_ready_idx = -1;

    return g_audio_samples;
}

/**
 * 获取最近一次缓冲区的音频峰值电平
 *
 * @return 峰值 (0 ~ 32767), 无数据时返回 0
 *
 * 注意: 必须先调用 inmp441_get_buffer() 刷新数据,
 *       否则返回的是上一次的值
 */
int32_t inmp441_get_peak(void)
{
    return g_audio_peak;
}

/**
 * 停止音频采集
 *
 * 停止 SSI1 和 GPT1, 关闭 I2S 接口
 *
 * @retval FSP_SUCCESS  停止成功
 * @retval 其他          失败
 */
fsp_err_t inmp441_stop(void)
{
    fsp_err_t err;

    g_capture_running = false;

    /* 停止 SSI1 (等待下一帧边界后进入 IDLE) */
    err = g_i2s1.p_api->stop(g_i2s1.p_ctrl);
    if (FSP_SUCCESS != err)
    {
        printf("INMP441: SSI1 stop failed! err=%d\r\n", err);
        return err;
    }

    /* 等待 SSI 进入 IDLE 状态 (回调中清除 g_capture_running) */
    uint32_t timeout = 100000;
    while (g_capture_running && timeout > 0)
    {
        R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MICROSECONDS);
        timeout--;
    }

    /* 关闭 SSI1 */
    err = g_i2s1.p_api->close(g_i2s1.p_ctrl);
    if (FSP_SUCCESS != err)
    {
        printf("INMP441: SSI1 close failed! err=%d\r\n", err);
        return err;
    }

    /* 关闭 GPT1 */
    err = g_timer1.p_api->close(g_timer1.p_ctrl);
    if (FSP_SUCCESS != err)
    {
        printf("INMP441: GPT1 close failed! err=%d\r\n", err);
        return err;
    }

    printf("INMP441: stopped\r\n");
    return FSP_SUCCESS;
}
