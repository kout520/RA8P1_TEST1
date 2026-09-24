/**
 * Speaker Verification — utterance-level MFCC mean matching
 *
 * Approach: each speaker is represented by a 13-dim mean MFCC vector.
 * Verification uses cosine similarity between the query utterance's
 * mean MFCC and each enrolled speaker's mean. Threshold tuned for
 * reasonable false-reject / false-accept tradeoff.
 *
 * Memory: SPK_MAX_ENROLLED × 13 floats = negligible
 */

#include "speaker_verify.h"
#include "mfcc_engine.h"
#include <string.h>
#include <math.h>
#include <stdio.h>

/* ==================================================================
 *                     Persistent State (.noinit survives reset)
 * ================================================================== */
#define SPK_MAGIC  0x53504B30  /* "SPK0" */

typedef struct {
    uint32_t magic;
    uint32_t checksum;         /* simple XOR of speaker data */
    int      count;
    float    mean[SPK_MAX_ENROLLED][SPK_N_FEAT];
    char     name[SPK_MAX_ENROLLED][16];
} spk_nvram_t;

/* Placed in .noinit section — survives warm reset, cleared only on power-cycle */
static spk_nvram_t g_nvram __attribute__((section(".noinit")));

/* Runtime state (rebuilt from g_nvram on init or after save) */
static float g_spk_mean[SPK_MAX_ENROLLED][SPK_N_FEAT];
static char  g_spk_name[SPK_MAX_ENROLLED][16];
static int   g_spk_count = 0;

/* Enrollment accumulator (before finalize) */
static float g_enroll_sum[SPK_N_FEAT];
static int   g_enroll_frames = 0;
static int   g_enroll_utt    = 0;

/* 余弦相似度阈值 (已剔除 C0, 只用 C1..C12 及其差分)。
   同一说话人通常 > 0.60, 不同说话人通常 < 0.45。
   注: 加一阶差分(26维)后 sim 值域会变, 阈值需现场校准。
   speaker_verify 返回 -1(灰区)/-2(硬拒) 的分界见 SPK_SIM_REJECT。 */
#define SPK_SIM_REJECT     0.45f
#define SPK_MARGIN         0.06f   /* 多人时 best 必须领先 second 这么多, 否则区分度不足 */

/* 声纹通过阈值 (串口屏可调, 默认 0.52) */
float g_spk_sim_threshold = 0.52f;

/* ==================================================================
 *                     Implementation
 * ================================================================== */

/**
 * Compute per-frame mean MFCC of an utterance.
 */
static void utterance_mean(const float *features, int n_frames, float *mean_out)
{
    memset(mean_out, 0, SPK_N_FEAT * sizeof(float));
    if (n_frames < 1) return;
    for (int f = 0; f < n_frames; f++) {
        const float *cur  = &features[f * SPK_N_MFCC];
        const float *prev = (f > 0)          ? &features[(f - 1) * SPK_N_MFCC] : cur;
        const float *next = (f < n_frames-1) ? &features[(f + 1) * SPK_N_MFCC] : cur;
        for (int k = 0; k < SPK_N_MFCC; k++) {
            mean_out[k]              += cur[k];
            mean_out[SPK_N_MFCC + k] += (next[k] - prev[k]) * 0.5f;  /* 一阶差分 */
        }
    }
    float inv = 1.0f / (float)n_frames;
    for (int k = 0; k < SPK_N_FEAT; k++) {
        mean_out[k] *= inv;
    }
}

/**
 * Cosine similarity between two 13-dim vectors.
 */
static float cosine_sim(const float *a, const float *b)
{
    float dot = 0.0f, na = 0.0f, nb = 0.0f;
    /* 跳过 C0 (对数能量) 和 delta-C0 (能量变化率):
       两者都随音量/距离大幅变化且不区分说话人。
       用静态 C1..C12 (声道形状/共振峰) + delta C1..C12 (声道动态) 共 24 维。 */
    for (int k = 1; k < SPK_N_MFCC; k++) {
        dot += a[k] * b[k];
        na  += a[k] * a[k];
        nb  += b[k] * b[k];
    }
    for (int k = SPK_N_MFCC + 1; k < SPK_N_FEAT; k++) {
        dot += a[k] * b[k];
        na  += a[k] * a[k];
        nb  += b[k] * b[k];
    }
    float denom = sqrtf(na * nb);
    if (denom < 1e-10f) return 0.0f;
    return dot / denom;
}

/* ==================================================================
 *                     Public API
 * ================================================================== */

int speaker_enroll(const float *features, int n_frames, const char *label)
{
    if (g_spk_count >= SPK_MAX_ENROLLED) return -1;

    /* Accumulate 26-dim (13 静态 MFCC + 13 一阶差分) frame-wise sum */
    for (int f = 0; f < n_frames; f++) {
        const float *cur  = &features[f * SPK_N_MFCC];
        const float *prev = (f > 0)          ? &features[(f - 1) * SPK_N_MFCC] : cur;
        const float *next = (f < n_frames-1) ? &features[(f + 1) * SPK_N_MFCC] : cur;
        for (int k = 0; k < SPK_N_MFCC; k++) {
            g_enroll_sum[k]              += cur[k];
            g_enroll_sum[SPK_N_MFCC + k] += (next[k] - prev[k]) * 0.5f;
        }
    }
    g_enroll_frames += n_frames;
    g_enroll_utt++;

    /* Store label (first enrollment call only) */
    if (g_enroll_utt == 1 && label != NULL) {
        strncpy(g_spk_name[g_spk_count], label, 15);
        g_spk_name[g_spk_count][15] = '\0';
    }

    printf("SPK: enrolled %s utt#%d frames=%d\r\n",
           g_spk_name[g_spk_count], g_enroll_utt, n_frames);
    return 0;
}

int speaker_finalize(void)
{
    if (g_enroll_frames < 1) return -1;

    float inv = 1.0f / (float)g_enroll_frames;
    for (int k = 0; k < SPK_N_FEAT; k++) {
        g_spk_mean[g_spk_count][k] = g_enroll_sum[k] * inv;
    }

    printf("SPK: %s finalized (%d utt, %d frames)\r\n",
           g_spk_name[g_spk_count], g_enroll_utt, g_enroll_frames);

    int idx = g_spk_count;
    g_spk_count++;

    /* Reset enrollment accumulator for next speaker */
    memset(g_enroll_sum, 0, sizeof(g_enroll_sum));
    g_enroll_frames = 0;
    g_enroll_utt    = 0;

    speaker_save();  /* persist to .noinit */
    return idx;
}

int speaker_verify(const float *features, int n_frames)
{
    if (g_spk_count == 0) return 0;  /* no speaker enrolled → pass all */
    if (n_frames < 10) return -2;

    /* 能量检查: C0 均值反映响度。静音/噪声的 C0 极低, C1..C12 接近 0,
       余弦会数值不稳定产生虚假高 sim → 直接拒绝 (与 voice_cmd_predict 同门槛) */
    {
        float c0_sum = 0.0f;
        for (int f = 0; f < n_frames; f++)
            c0_sum += features[f * SPK_N_MFCC];  /* C0 是第一个系数 */
        float c0_mean = c0_sum / (float)n_frames;
        if (c0_mean > 0.0f || c0_mean < -11.0f) {
            return -2;  /* 静音或噪声 */
        }
    }

    /* C1..C12 能量检查: 纯噪声频谱平坦, C1..C12 接近 0(真人语音每帧平方和≈100~200),
       C1..C12 过小 → 余弦分母趋近 0, 数值不稳定产生虚假高 sim → 拒绝 */
    {
        float c_energy = 0.0f;
        for (int f = 0; f < n_frames; f++) {
            const float *fr = &features[f * SPK_N_MFCC];
            for (int k = 1; k < SPK_N_MFCC; k++) {
                c_energy += fr[k] * fr[k];
            }
        }
        c_energy /= (float)n_frames;
        if (c_energy < 20.0f) {
            return -2;  /* 噪声/静音 */
        }
    }

    float query_mean[SPK_N_FEAT];
    utterance_mean(features, n_frames, query_mean);

    float best_sim   = -1.0f;
    float second_sim = -1.0f;
    int   best_idx   = -1;

    for (int i = 0; i < g_spk_count; i++) {
        float sim = cosine_sim(query_mean, g_spk_mean[i]);
        if (sim > best_sim) {
            second_sim = best_sim;
            best_sim   = sim;
            best_idx   = i;
        } else if (sim > second_sim) {
            second_sim = sim;
        }
    }

    /* 多人时 margin 判据: best 必须明显领先 second。
       两人都念相同指令词时, 均值声纹(含内容信息)会彼此靠近,
       best/second 差距太小 → 无法可靠区分身份 → 拒绝, 避免 A 被认成 B。 */
    if (g_spk_count >= 2 && (best_sim - second_sim) < SPK_MARGIN) {
        if (best_sim > 0.40f)
            printf("\r\n[SPK] ambiguous best=%.3f u%d 2nd=%.3f\r\n",
                   (double)best_sim, best_idx, (double)second_sim);
        return -2;  /* 区分度不足, 拒绝 (不 dispatch, 避免身份误判) */
    }

    if (best_sim >= g_spk_sim_threshold) {
        printf("\r\n[SPK] OK sim=%.3f user=%d 2nd=%.3f\r\n",
               (double)best_sim, best_idx, (double)second_sim);
        return best_idx;  /* verified */
    }
    if (best_sim >= SPK_SIM_REJECT) {
        return -1;  /* 灰区 [0.45, 0.52): 命令必须极确信才放行 */
    }
    return -2;  /* 硬拒 sim < 0.45 (静默, 环境噪声频繁触发不打印) */
}

int  speaker_get_count(void)    { return g_spk_count; }
bool speaker_has_enrolled(void) { return g_spk_count > 0; }

/* ==================================================================
 *          Non-volatile save / load (.noinit section)
 * ================================================================== */

static uint32_t nvram_checksum(void)
{
    uint32_t cs = 0;
    uint8_t *p = (uint8_t *)&g_nvram.count;
    size_t  len = sizeof(g_nvram) - offsetof(spk_nvram_t, count);
    for (size_t i = 0; i < len; i++) cs ^= p[i];
    return cs;
}

/* Serialize enrolled speakers to a compact blob for MRAM */
static uint32_t spk_serialize(uint8_t *buf, uint32_t max_len)
{
    uint32_t pos = 0;
    if (pos + 4 > max_len) return 0;
    memcpy(&buf[pos], &g_spk_count, 4); pos += 4;     /* count */
    for (int i = 0; i < g_spk_count; i++) {
        uint32_t sz = SPK_N_FEAT * sizeof(float) + 16;
        if (pos + sz > max_len) return 0;
        memcpy(&buf[pos], g_spk_mean[i], SPK_N_FEAT * sizeof(float));
        pos += SPK_N_FEAT * sizeof(float);
        memcpy(&buf[pos], g_spk_name[i], 16); pos += 16;
    }
    return pos;
}

/* Deserialize blob → enrolled speakers */
static bool spk_deserialize(const uint8_t *buf, uint32_t len)
{
    if (len < 4) return false;
    uint32_t pos = 0;
    memcpy(&g_spk_count, &buf[pos], 4); pos += 4;
    if (g_spk_count > SPK_MAX_ENROLLED) { g_spk_count = 0; return false; }
    for (int i = 0; i < g_spk_count; i++) {
        if (pos + SPK_N_FEAT * 4 + 16 > len) { g_spk_count = 0; return false; }
        memcpy(g_spk_mean[i], &buf[pos], SPK_N_FEAT * sizeof(float));
        pos += SPK_N_FEAT * sizeof(float);
        memcpy(g_spk_name[i], &buf[pos], 16); pos += 16;
    }
    return true;
}

void speaker_save(void)
{
    /* 保存到 .noinit RAM (热复位恢复) */
    g_nvram.magic    = SPK_MAGIC;
    g_nvram.count    = g_spk_count;
    memcpy(g_nvram.mean, g_spk_mean, sizeof(g_spk_mean));
    memcpy(g_nvram.name, g_spk_name, sizeof(g_spk_name));
    g_nvram.checksum = nvram_checksum();

    /* 断电恢复改用 ESP32 存储 (见 speaker_export_hex, hal_entry.c 里发 @SPK) */
    printf("SPK: saved %d speaker(s)\r\n", g_spk_count);
}

/* 导出声纹为 hex 字符串 (供 ESP32 存储, 断电恢复) */
void speaker_export_hex(char *out, int max_len)
{
    uint8_t blob[1024];
    uint32_t len = spk_serialize(blob, sizeof(blob));
    int pos = 0;
    for (uint32_t i = 0; i < len && pos < max_len - 2; i++) {
        pos += sprintf(&out[pos], "%02X", blob[i]);
    }
    out[pos] = '\0';
}

/* 从 hex 字符串导入声纹 (ESP32 发回时调用) */
bool speaker_import_hex(const char *hex)
{
    if (NULL == hex) return false;
    int hexlen = (int)strlen(hex);
    if (hexlen < 2 || (hexlen & 1)) return false;  /* 非偶数长度 */

    uint8_t blob[1024];
    int len = hexlen / 2;
    if (len > (int)sizeof(blob)) return false;

    for (int i = 0; i < len; i++) {
        unsigned int v = 0;
        sscanf(&hex[i * 2], "%2X", &v);
        blob[i] = (uint8_t)v;
    }

    if (!spk_deserialize(blob, (uint32_t)len)) return false;

    /* 同步到 .noinit RAM (热复位也能恢复) */
    g_nvram.magic    = SPK_MAGIC;
    g_nvram.count    = g_spk_count;
    memcpy(g_nvram.mean, g_spk_mean, sizeof(g_spk_mean));
    memcpy(g_nvram.name, g_spk_name, sizeof(g_spk_name));
    g_nvram.checksum = nvram_checksum();
    printf("SPK: imported %d speaker(s) from ESP32\r\n", g_spk_count);
    return true;
}

bool speaker_load(void)
{
    /* 从 .noinit RAM 恢复 (热复位); 断电恢复由 ESP32 发 @SPK 完成 */
    if (g_nvram.magic == SPK_MAGIC && nvram_checksum() == g_nvram.checksum) {
        g_spk_count = g_nvram.count;
        memcpy(g_spk_mean, g_nvram.mean, sizeof(g_spk_mean));
        memcpy(g_spk_name, g_nvram.name, sizeof(g_spk_name));
        printf("SPK: loaded %d speaker(s) from RAM\r\n", g_spk_count);
        return true;
    }

    printf("SPK: no saved data\r\n");
    return false;
}

void speaker_erase(void)
{
    memset(&g_nvram, 0, sizeof(g_nvram));
    g_spk_count = 0;
    memset(g_spk_mean, 0, sizeof(g_spk_mean));
    memset(g_spk_name, 0, sizeof(g_spk_name));
    printf("SPK: erased\r\n");
}
