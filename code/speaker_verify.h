/**
 * Speaker Verification — utterance-level MFCC mean matching
 *
 * Each enrolled speaker is represented by a 13-dim mean MFCC vector
 * computed from several enrollment utterances. During recognition,
 * the current utterance's mean MFCC is compared against all enrolled
 * speakers via cosine similarity. If no match exceeds threshold, the
 * utterance is rejected before DTW command matching.
 */

#ifndef __SPEAKER_VERIFY_H__
#define __SPEAKER_VERIFY_H__

#include <stdint.h>
#include <stdbool.h>

#define SPK_MAX_ENROLLED   4     /* max 4 authorized speakers */
#define SPK_N_MFCC         13    /* MFCC 输入维度 (DTW_N_MFCC) */
#define SPK_N_FEAT         26    /* 声纹特征维度 = 13 静态 MFCC + 13 一阶差分(delta) */

/* 声纹通过阈值 (串口屏可调, 默认 0.52) */
extern float g_spk_sim_threshold;

/**
 * Enroll a new speaker from an utterance's MFCC features.
 * Call once per enrollment recording (3-5 times per speaker).
 * After all enrollment calls, call speaker_finalize() to lock the model.
 *
 * @param features  MFCC array [n_frames * 13]
 * @param n_frames  number of frames in this utterance
 * @param label     speaker name (max 15 chars)
 * @return 0 on success, -1 if enrollment table full
 */
int  speaker_enroll(const float *features, int n_frames, const char *label);

/**
 * Finalize the current enrollment: compute the mean vector and make it active.
 * Call after 3-5 speaker_enroll() calls for one speaker.
 * @return speaker index (0..SPK_MAX_ENROLLED-1), -1 on error
 */
int  speaker_finalize(void);

/**
 * Verify: does this utterance belong to an enrolled speaker?
 *
 * @param features  MFCC array [n_frames * 13]
 * @param n_frames  number of frames
 * @return speaker index (0..SPK_MAX_ENROLLED-1) if verified (sim >= 0.52),
 *         -1 if gray zone (0.45 <= sim < 0.52, 需命令极确信才放行),
 *         -2 if hard reject (sim < 0.45, 陌生声音)
 */
int  speaker_verify(const float *features, int n_frames);

/**
 * Get enrolled speaker count
 */
int  speaker_get_count(void);

/**
 * Check if a speaker has been enrolled and finalized
 */
bool speaker_has_enrolled(void);

/** Non-volatile save/load/erase (.noinit — survives reset, not power-cycle) */
void speaker_save(void);
bool speaker_load(void);
void speaker_erase(void);

/** 导出/导入声纹为 hex 字符串 (用于 ESP32 存储, 断电恢复) */
void speaker_export_hex(char *out, int max_len);
bool speaker_import_hex(const char *hex);

#endif
