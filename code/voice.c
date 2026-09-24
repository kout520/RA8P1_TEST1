/**
 * 天问语音模块 - 实现文件
 * 通过 USART1 (115200bps, 8N1) 发送命令控制语音播报
 *
 * 依赖: FSP UART API (g_uart1), headfile.h
 */

#include "voice.h"
#include "headfile.h"
#include <string.h>

/* ==================================================================
 *                        UART 发送
 * ================================================================== */

static volatile uint8_t g_uart1_tx_done = 1;

static void voice_wait_tx(void)
{
    uint32_t timeout = 100000;  /* 超时保护, 防止语音模块未连接时死锁 */
    while (!g_uart1_tx_done && --timeout) {
        ;
    }
    g_uart1_tx_done = 0;
}

/** 发送单个字符 */
static void voice_send_char(char ch)
{
    uint8_t byte = (uint8_t)ch;
    g_uart1_tx_done = 0;
    g_uart1.p_api->write(g_uart1.p_ctrl, &byte, 1);
    voice_wait_tx();
}

/** 发送原始字符串 (不追加结束符) */
static void voice_send_raw(const char *str)
{
    if (NULL == str) return;
    while (*str != '\0') {
        voice_send_char(*str++);
    }
}

/* ==================================================================
 *                        公共接口
 * ================================================================== */

/**
 * 发送字符串到语音模块 (自动追加 \r\n)
 */
void voice_send_string(const char *str)
{
    if (NULL == str) return;
    voice_send_raw(str);
    voice_send_char('\r');
    voice_send_char('\n');

    /* 播报后设置盲区: 防止麦克风拾取播报声形成反馈循环。
       覆盖所有播报路径(语音命令/关柜门/NFC等), 统一在这里屏蔽识别。 */
    g_voice_blind_until = msTicks + VOICE_BLIND_MS;
}

/* ==================================================================
 *                        延迟播报队列 (非阻塞)
 * 用于"先播报 A, 2~3秒后再播报 B", 避免 B 打断 A
 * ================================================================== */
#define VOICE_PENDING_MAX  4
typedef struct {
    char     str[16];
    uint32_t send_time;   /* 到点发送的时间戳 (msTicks) */
} voice_pending_t;

static voice_pending_t g_voice_pending[VOICE_PENDING_MAX];
static int             g_voice_pending_cnt = 0;

void voice_send_delayed(const char *str, uint32_t delay_ms)
{
    if (NULL == str || g_voice_pending_cnt >= VOICE_PENDING_MAX) return;
    strncpy(g_voice_pending[g_voice_pending_cnt].str, str, 15);
    g_voice_pending[g_voice_pending_cnt].str[15] = '\0';
    g_voice_pending[g_voice_pending_cnt].send_time = msTicks + delay_ms;
    g_voice_pending_cnt++;
}

void voice_pending_poll(void)
{
    for (int i = 0; i < g_voice_pending_cnt; ) {
        if (msTicks >= g_voice_pending[i].send_time) {
            voice_send_string(g_voice_pending[i].str);
            g_voice_pending[i] = g_voice_pending[g_voice_pending_cnt - 1];  /* 尾元素填补 */
            g_voice_pending_cnt--;
        } else {
            i++;
        }
    }
}

/**
 * 发送考勤播报命令: @1 ~ @6\r\n
 * 对应播报不同人员的打卡语音
 */
void voice_send_attendance(uint8_t id)
{
    if (id < 1 || id > 6) return;

    char buf[4];
    sprintf(buf, "@%d", id);
    voice_send_string(buf);
    printf("Voice: attendance %s\r\n", buf);
}

/* ==================================================================
 *                        UART 回调
 * ================================================================== */

/**
 * UART1 发送完成回调
 * 仅关注 TX_COMPLETE 事件
 */
void uart1_callback(uart_callback_args_t *p_args)
{
    if (NULL == p_args) return;

    if (UART_EVENT_TX_COMPLETE == p_args->event) {
        g_uart1_tx_done = 1;
    }
}

/* ==================================================================
 *                        初始化
 * ================================================================== */

/**
 * 初始化语音模块通信
 * 1. 打开 USART1 (115200bps, 8N1)
 */
void Voice_Init(void)
{
    fsp_err_t err;

    /* 打开 UART1 */
    err = g_uart1.p_api->open(g_uart1.p_ctrl, g_uart1.p_cfg);
    if (FSP_SUCCESS != err) {
        printf("Voice: UART1 open failed! err=%d\r\n", err);
        return;
    }

    g_uart1_tx_done = 1;

    printf("Voice: USART1 init done (115200bps)\r\n");


}
