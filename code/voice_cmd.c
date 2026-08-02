/**
 * Voice Command Inference — Logistic Regression on RA8P1
 *
 * Model: Logistic Regression (coefficients from quick_train.py)
 * Input: 1950 MFCC features (150 frames × 13 coefficients)
 * Output: 0 = kai_ji, 1 = guan_ji
 */

#include "voice_cmd.h"
#include "mfcc_engine.h"
#include "model_2cmd.h"
#include <math.h>
#include <stdio.h>

/* ==================================================================
 *                     Static State
 * ================================================================== */

static float g_last_score = 0.0f;
static float g_last_prob  = 0.0f;
static int   g_last_pred  = -1;
static bool  g_inference_done = false;

static const char * g_cmd_names[] = {
    [VOICE_CMD_KAI_JI]   = "kai_ji (open)",
    [VOICE_CMD_GUAN_JI]  = "guan_ji (close)",
};

/* ==================================================================
 *                     Public API
 * ================================================================== */

void voice_cmd_init(void)
{
    g_last_score = 0.0f;
    g_last_prob  = 0.0f;
    g_last_pred  = -1;
    g_inference_done = false;
}

/**
 * Logistic regression inference:
 *   score = bias + dot(weights, features)
 *   prob  = sigmoid(score)
 *   pred  = score > 0 ? 1 : 0
 */
int voice_cmd_predict(const float *features)
{
    /* Compute dot product: bias + sum(weight[i] * feature[i]) */
    float score = MODEL_BIAS;

    for (int i = 0; i < MFCC_N_FEATURES; i++) {
        score += model_weights[i] * features[i];
    }

    /* Sigmoid for probability */
    float prob = 1.0f / (1.0f + expf(-score));

    /* Confidence thresholds */
    #define VOICE_THRESH_HIGH  0.55f   /* prob>0.55 → guan_ji */
    #define VOICE_THRESH_LOW   0.45f   /* prob<0.45 → kai_ji */

    int pred;
    const char *label;

    static int  cooldown = 0;
    static int  last_pred = -1;
    static int  confirm_cnt = 0;

    if (prob > VOICE_THRESH_HIGH) {
        pred = VOICE_CMD_GUAN_JI;
    } else if (prob < VOICE_THRESH_LOW) {
        pred = VOICE_CMD_KAI_JI;
    } else {
        pred = -1;
        confirm_cnt = 0;
    }

    /* Cooldown after trigger */
    if (cooldown > 0) {
        cooldown--;
        if (pred < 0) { g_last_pred = -1; return -1; }
        return pred;
    }

    /* Require 2 consecutive same predictions to trigger */
    if (pred >= 0 && pred == last_pred) {
        confirm_cnt++;
        if (confirm_cnt >= 2) {
            printf("\r\n>> %s << (%.0f%%)\r\n",
                   g_cmd_names[pred], (double)(prob * 100.0f));
            cooldown = 3;   /* ~4.5 sec cooldown (3 inferences × 1.5s) */
            confirm_cnt = 0;
        }
    } else if (pred >= 0) {
        confirm_cnt = 1;
    }
    last_pred = pred;

    g_last_score = score;
    g_last_prob  = prob;
    g_last_pred  = pred;

    return pred;
}

bool voice_cmd_is_ready(void)
{
    return g_inference_done;
}

void voice_cmd_reset(void)
{
    g_inference_done = false;
}

int voice_cmd_get_last(void)
{
    return g_last_pred;
}
