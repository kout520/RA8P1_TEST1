/**
 * 天问语音模块 - 头文件
 * 通过 USART1 (115200bps, 8N1) 发送命令控制语音播报
 *
 * 协议:
 *   @1\r\n   张三打卡
 *   @2\r\n   李四打卡
 *   @3\r\n   王五打卡
 *   @4\r\n   张三(微笑)打卡
 *   @5\r\n   李四(平静)打卡
 *   @6\r\n   王五(高兴)打卡
 *
 * 注: 此模块仅发送, 不接收语音模块数据
 */
#ifndef __VOICE_H__
#define __VOICE_H__

#include <stdint.h>

/* ===== 公共接口 ===== */

/** 初始化语音模块 (打开 USART1, 115200bps) */
void Voice_Init(void);

/** 发送考勤播报命令: @1 ~ @6\r\n */
void voice_send_attendance(uint8_t id);

/** 发送原始字符串到语音模块 */
void voice_send_string(const char *str);

#endif /* __VOICE_H__ */
