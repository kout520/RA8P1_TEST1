/**
 * Voice Command Inference — Logistic Regression
 *
 * Simple logistic regression classifier:
 *   score = bias + sum(weight[i] * feature[i])
 *   prob = 1 / (1 + exp(-score))
 *   pred = 0 if prob < 0.5 else 1
 *
 * Commands: 0 = kai_ji (power on), 1 = guan_ji (power off)
 */

#ifndef __VOICE_CMD_H__
#define __VOICE_CMD_H__

#include <stdint.h>
#include <stdbool.h>

#define VOICE_CMD_KAI_JI    0
#define VOICE_CMD_GUAN_JI   1

void voice_cmd_init(void);
int  voice_cmd_predict(const float *features);
bool voice_cmd_is_ready(void);
void voice_cmd_reset(void);
int  voice_cmd_get_last(void);

#endif
