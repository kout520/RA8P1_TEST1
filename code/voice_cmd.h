/**
 * Voice Command — DTW template matching
 */

#ifndef __VOICE_CMD_H__
#define __VOICE_CMD_H__

#include <stdint.h>
#include <stdbool.h>

void voice_cmd_init(void);
int  voice_cmd_predict(const float *features, int n_frames);
bool voice_wake_detect(const float *features, int n_frames);  /* 唤醒词"大白"检测 */
bool voice_cmd_is_ready(void);
void voice_cmd_reset(void);
int  voice_cmd_get_last(void);

/* 灰区声纹时额外收紧 DTW margin (由 hal_entry 设置, 见 voice_cmd.c) */
extern float g_dtw_extra_margin;

/* 串口屏可调的识别参数 (默认值见 voice_cmd.c) */
extern float g_th_base;         /* 指令 DTW 通过阈值 (灵敏度) */
extern float g_dtw_margin;      /* best 领先 second 的最小差距 (唯一性/抗误识别) */
extern float g_wake_threshold;  /* 唤醒词阈值 */

#endif
