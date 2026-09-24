/*
 * headfile.h
 *
 *  Created on: 2026年5月16日
 *      Author: kout
 */

#ifndef HEADFILE_H_
#define HEADFILE_H_

#include <stdint.h>
#include "bsp_api.h"
#include "common_data.h"
#include "r_gpt.h"
#include "r_timer_api.h"
#include "r_iic_master.h"
#include "r_i2c_master_api.h"
#include "r_capture_api.h"
#include "r_ceu.h"
#include "r_sci_b_uart.h"
#include "r_uart_api.h"
#include "hal_data.h"



#include "tjc_usart_hmi.h"
#include "key.h"
#include "lcd.h"
#include "AS608pro.h"
#include "pn532.h"
#include "esp32_comm.h"
#include "voice.h"
#include "inmp441.h"
#include "mfcc_engine.h"
#include "voice_cmd.h"

/* ===== 跨模块共享变量声明 ===== */
extern volatile int motor_flag;                             // 开锁标志
extern int g_current_user;                                  // 当前认证用户 (0=无,1=张三,2=李四,3=王五,4=老六)
extern volatile uint32_t msTicks;                           // 全局1ms计数 (hal_entry.c)
extern volatile uint32_t g_voice_blind_until;               // 语音播报盲区截止 (hal_entry.c)
extern int g_vad_th_min;                                    // VAD 触发灵敏度 (hal_entry.c, 串口屏可调, 默认 1800)

/* 语音播报后屏蔽识别时长(ms): 麦克风拾取播报声会形成反馈循环 */
#define VOICE_BLIND_MS  8000

/* ===== 跨模块函数声明 ===== */
void uart2_send_value(int val);                             // USART2 发送(ESP32)
void uart8_send_value(int val);                             // USART8 发送(舵机/摄像头)
void uart9_send_value(int val);                             // USART9 发送(调试)
void dakai_clock_display(void);                             // 时钟页面显示
/* 颜色检测 (跨文件共享) */
typedef enum { COLOR_NONE, COLOR_RED, COLOR_GREEN, COLOR_BLUE } color_t;
const char* color_name(color_t c);
color_t detect_color(uint16_t *image, uint32_t w, uint32_t h);


#endif /* HEADFILE_H_ */
