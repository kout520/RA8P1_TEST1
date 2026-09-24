/**
 * Voice Command — Standard DTW template matching (utterance-level)
 *
 * Whole-utterance matching with cosine-distance DTW.
 * Minimum frame ratio check prevents short queries from
 * producing spurious matches against long templates.
 */

#include "voice_cmd.h"
#include "mfcc_engine.h"
#include "templates_dtw.h"
#include "wake_word.h"
#include <arm_mve.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

/* DTW buffers: 2 rows × template_len (memory-optimized) */
static float g_dtw_buf[2][DTW_MAX_TMPL_FRAMES];
static float g_q_norm[DTW_MAX_TMPL_FRAMES * DTW_N_MFCC];
static float g_t_norm[DTW_MAX_TMPL_FRAMES * DTW_N_MFCC];

/* 灰区声纹时额外收紧 margin (hal_entry 在声纹灰区时置 0.04, 总 margin=0.12) */
float g_dtw_extra_margin = 0.0f;

/* 串口屏可调识别参数 (默认值 = 原 #define) */
float g_th_base        = 0.32f;  /* 指令灵敏度阈值 */
float g_dtw_margin     = 0.08f;  /* 唯一性 margin */
float g_wake_threshold = 0.30f;  /* 唤醒词阈值 */

void voice_cmd_init(void) { }

/**
 * 13-dim dot product — MVE-accelerated (4×float per cycle).
 * DTW_N_MFCC=13 → 3×MVE vectors (12 elements) + 1 scalar.
 */
static inline float dot13(const float *__restrict a, const float *__restrict b)
{
    float32x4_t sum4 = vdupq_n_f32(0.0f);
    /* 12 elements: 3 × 4-wide MVE multiply-accumulate */
    for (int k = 0; k < 12; k += 4) {
        sum4 = vfmaq(sum4, vld1q(&a[k]), vld1q(&b[k]));
    }
    /* Reduce 4-lane sum to scalar, add 13th element */
    float s = vgetq_lane(sum4, 0) + vgetq_lane(sum4, 1)
            + vgetq_lane(sum4, 2) + vgetq_lane(sum4, 3);
    return s + a[12] * b[12];
}

/**
 * L2-normalize each MFCC frame in-place
 */
static void norm_frames(float *seq, int n_frames)
{
    for (int f = 0; f < n_frames; f++) {
        float *frame = &seq[f * DTW_N_MFCC];
        float sum_sq = dot13(frame, frame);
        float inv = 1.0f / sqrtf(sum_sq + 1e-10f);
        for (int k = 0; k < DTW_N_MFCC; k++) frame[k] *= inv;
    }
}

/**
 * Standard DTW: full-sequence matching, normalized by (q_len + t_len).
 */
static float dtw_std(const float *q, int q_len,
                     const float *t, int t_len)
{
    g_dtw_buf[0][0] = 1.0f - dot13(&q[0], &t[0]);
    for (int j = 1; j < t_len; j++) {
        float d = dot13(&q[0], &t[j * DTW_N_MFCC]);
        g_dtw_buf[0][j] = g_dtw_buf[0][j-1] + (1.0f - d);
    }
    int cur = 1, prev = 0;
    for (int i = 1; i < q_len; i++) {
        const float *qi = &q[i * DTW_N_MFCC];
        g_dtw_buf[cur][0] = g_dtw_buf[prev][0] + (1.0f - dot13(qi, &t[0]));
        for (int j = 1; j < t_len; j++) {
            float cost = 1.0f - dot13(qi, &t[j * DTW_N_MFCC]);
            float min_v = g_dtw_buf[prev][j];
            if (g_dtw_buf[cur][j-1] < min_v) min_v = g_dtw_buf[cur][j-1];
            if (g_dtw_buf[prev][j-1] < min_v) min_v = g_dtw_buf[prev][j-1];
            g_dtw_buf[cur][j] = cost + min_v;
        }
        cur ^= 1; prev ^= 1;
    }
    return g_dtw_buf[prev][t_len-1] / (float)(q_len + t_len);
}

/**
 * Subsequence DTW: find best template match anywhere within the query.
 *
 * Unlike standard DTW, the template can match a sub-segment of the query.
 * This enables keyword spotting in longer sentences.
 *
 * Key differences:
 *   - First column: NO accumulation (matching can start at any query frame)
 *   - Final score: minimum of last column / template_length
 */
static float dtw_sub(const float *q, int q_len,
                     const float *t, int t_len)
{
    if (t_len < 1 || q_len < 1) return 1e10f;

    /* Row 0 */
    {
        const float *q0 = &q[0];
        g_dtw_buf[0][0] = 1.0f - dot13(q0, &t[0]);
        for (int j = 1; j < t_len; j++) {
            g_dtw_buf[0][j] = g_dtw_buf[0][j-1]
                            + (1.0f - dot13(q0, &t[j * DTW_N_MFCC]));
        }
    }

    int cur = 1, prev = 0;
    float best_end = g_dtw_buf[0][t_len - 1];

    for (int i = 1; i < q_len; i++) {
        const float *qi = &q[i * DTW_N_MFCC];

        /* First column: can START matching at any query frame */
        g_dtw_buf[cur][0] = 1.0f - dot13(qi, &t[0]);  /* no accumulation */

        /* Remaining columns: standard DTW recurrence */
        for (int j = 1; j < t_len; j++) {
            float cost = 1.0f - dot13(qi, &t[j * DTW_N_MFCC]);
            float min_v = g_dtw_buf[prev][j];
            if (g_dtw_buf[cur][j-1] < min_v) min_v = g_dtw_buf[cur][j-1];
            if (g_dtw_buf[prev][j-1] < min_v) min_v = g_dtw_buf[prev][j-1];
            g_dtw_buf[cur][j] = cost + min_v;
        }

        /* Track minimum at template END (last column) */
        if (g_dtw_buf[cur][t_len - 1] < best_end)
            best_end = g_dtw_buf[cur][t_len - 1];

        cur ^= 1;
        prev ^= 1;
    }

    return best_end / (float)t_len;
}


int voice_cmd_predict(const float *feat, int n_frames)
{
    if (n_frames < 15) {
        return -1;  /* too short, silent */
    }

    /* Energy-based noise rejection: compute C0 mean.
       Pure noise has very low C0 energy (< -8.0 typically).
       Real speech even at low volume has C0 > -6.0. */
    {
        float c0_sum = 0.0f;
        for (int f = 0; f < n_frames; f++)
            c0_sum += feat[f * DTW_N_MFCC];  /* C0 is first coefficient */
        float c0_mean = c0_sum / n_frames;
        if (c0_mean > 0.0f || c0_mean < -11.0f) {
            /* Extreme C0 values → likely noise or no real speech */
            return -1;
        }
    }

    /* Copy and normalize query */
    int q_total = n_frames * DTW_N_MFCC;
    memcpy(g_q_norm, feat, q_total * sizeof(float));
    norm_frames(g_q_norm, n_frames);

    /* Compare against each template */
    float best_dist   = 1e10f;
    int   best_class  = -1;
    float second_dist = 1e10f;
    int   second_class = -1;
    bool  best_is_sub = false;  /* track which DTW variant won */

    for (int c = 0; c < DTW_N_CLASSES; c++) {
        for (int t = 0; t < DTW_N_TEMPLATES; t++) {
            int t_frames = dtw_tmpl_frames[c][t];
            if (t_frames == 0) continue;

            /* Reject only if query is < 1/4 of template length (was /3, too aggressive) */
            if (n_frames < t_frames / 4) continue;

            int t_total = t_frames * DTW_N_MFCC;
            memcpy(g_t_norm, dtw_templates[c][t], t_total * sizeof(float));
            norm_frames(g_t_norm, t_frames);

            /* 长句: 查询比模板长 10 帧(0.1s)以上就用于序列匹配,
               在句尾窗口里找关键词 (如"我要打卡"里的"打卡") */
            float d;
            bool  used_sub = false;
            #define TAIL_WINDOW  180
            if (n_frames > t_frames + 10) {
                int tail_len = TAIL_WINDOW;
                if (tail_len > n_frames) tail_len = n_frames;
                const float *tail = &g_q_norm[(n_frames - tail_len) * DTW_N_MFCC];
                d = dtw_sub(tail, tail_len, g_t_norm, t_frames);
                used_sub = true;
            } else {
                d = dtw_std(g_q_norm, n_frames, g_t_norm, t_frames);
            }
            if (d < best_dist) {
                second_dist  = best_dist;
                second_class = best_class;
                best_dist    = d;
                best_class   = c;
                best_is_sub  = used_sub;
            } else if (d < second_dist) {
                second_dist  = d;
                second_class = c;
            }
        }
    }

    /* Per-class thresholds: scaled by template length.
       Short templates → higher threshold (less strict),
       because DTW distance normalization by (q_len+t_len)
       or t_len produces larger values for short templates.
       Formula: base * sqrt(avg_tpl_frames / 120)  */
    float g_th[DTW_N_CLASSES];
    /* 指令灵敏度阈值 g_th_base (串口屏可调), 按模板长度缩放 */
    for (int c = 0; c < DTW_N_CLASSES; c++) {
        int sum_f = 0, n_tpl = 0;
        for (int t = 0; t < DTW_N_TEMPLATES; t++) {
            int f = dtw_tmpl_frames[c][t];
            if (f > 0) { sum_f += f; n_tpl++; }
        }
        float avg_f = (n_tpl > 0) ? (float)sum_f / n_tpl : 120.0f;
        g_th[c] = g_th_base * sqrtf(avg_f / 120.0f);
        if (g_th[c] < 0.10f) g_th[c] = 0.10f;
        if (g_th[c] > 0.60f) g_th[c] = 0.60f;
    }

    /* ---- Decide: noise / ambiguous / trigger ---- */
    /* 唯一性 margin 用串口屏可调的 g_dtw_margin */

    if (best_class < 0 || best_dist >= 0.60f) {
        return -1;  /* pure noise, silent */
    }

    const char *name = g_dtw_names[best_class];

    /* Ambiguity check: different classes too close → always reject */
    if (second_class >= 0 && second_class != best_class
        && (best_dist + g_dtw_margin + g_dtw_extra_margin) >= second_dist) {
        const char *name2 = g_dtw_names[second_class];
        printf("\r\n[?] %s~%s d1=%.3f d2=%.3f\r\n", name, name2,
               (double)best_dist, (double)second_dist);
        return -1;  /* ambiguous, discard */
    }

    /* dtw_sub normalizes by only t_len (vs dtw_std's q_len+t_len),
       so its distances are ~1.4x higher. Scale threshold to compensate. */
    float th_eff = g_th[best_class];
    if (best_is_sub) th_eff *= 1.4f;

    /* ---- Acceptance policy: 只有 >> (d < th) 才算识别, [~]/[?] 都拒绝 ---- */
    if (best_dist < th_eff) {
        /* TRIGGER */
        printf("\r\n>> %s <<  d=%.3f th=%.3f 2nd=%.3f%s\r\n", name,
               (double)best_dist, (double)th_eff, (double)second_dist,
               best_is_sub ? " sub" : "");
        return best_class;
    }

    /* [~] 模糊匹配: 拒绝 (只有 >> 才算) */
    printf("\r\n[~] %s d=%.3f th=%.3f%s\r\n", name,
           (double)best_dist, (double)th_eff,
           best_is_sub ? " sub" : "");
    return -1;
}

/**
 * Wake word detection — "大白"
 * 用标准 DTW 匹配唤醒词模板, d < 阈值则唤醒。
 */
bool voice_wake_detect(const float *feat, int n_frames)
{
    if (n_frames < 15) return false;

    /* C0 能量检查: 唤醒词通常大声说, C0 偏高(实测"大白"C0≈-1.4~-2.5),
       上限放宽到 0.0(只挡削波), 下限 -11.0 挡静音噪声 */
    {
        float c0_sum = 0.0f;
        for (int f = 0; f < n_frames; f++)
            c0_sum += feat[f * DTW_N_MFCC];
        float c0_mean = c0_sum / (float)n_frames;
        if (c0_mean > 0.0f || c0_mean < -11.0f) return false;
    }

    /* 归一化查询 */
    int q_total = n_frames * DTW_N_MFCC;
    memcpy(g_q_norm, feat, q_total * sizeof(float));
    norm_frames(g_q_norm, n_frames);

    /* DTW 匹配唤醒词模板, 取最小距离 */
    float best_dist = 1e10f;
    for (int t = 0; t < WAKE_N_TEMPLATES; t++) {
        int t_frames = wake_tmpl_frames[t];
        if (t_frames == 0) continue;
        if (n_frames < t_frames / 4) continue;

        int t_total = t_frames * DTW_N_MFCC;
        memcpy(g_t_norm, wake_templates[t], t_total * sizeof(float));
        norm_frames(g_t_norm, t_frames);

        float d = dtw_std(g_q_norm, n_frames, g_t_norm, t_frames);
        if (d < best_dist) best_dist = d;
    }

    if (best_dist < g_wake_threshold) {
        printf("\r\n[WAKE] 大白 d=%.3f\r\n", (double)best_dist);
        return true;
    }
    return false;
}

bool voice_cmd_is_ready(void) { return true; }
void voice_cmd_reset(void) { }
int  voice_cmd_get_last(void) { return 0; }
