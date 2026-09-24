/**
 * MFCC Feature Extractor for RA8P1 (CMSIS-DSP)
 *
 * Parameters (for 8347 Hz sample rate):
 *   - Frame: 25ms window (209 samples), zero-padded to 512-pt FFT
 *   - Hop:   10ms (84 samples)
 *   - Mel filters: 40, covering 20Hz ~ 4173Hz
 *   - MFCC: 13 coefficients (classic speech recognition)
 *   - Buffer: 150 frames = 1.5 seconds
 *
 * Call flow:
 *   mfcc_engine_init() once
 *   mfcc_engine_feed(samples, count) for each audio buffer from INMP441
 *   mfcc_engine_ready() returns true when 150 frames collected
 *   mfcc_engine_get_features() returns float[150*13]
 */

#ifndef __MFCC_ENGINE_H__
#define __MFCC_ENGINE_H__

#include <stdint.h>
#include <stdbool.h>

/* MFCC Parameters */
#define MFCC_SAMPLE_RATE    8347
#define MFCC_FRAME_MS       25
#define MFCC_HOP_MS         10
#define MFCC_FRAME_SAMPLES  209     /* 25ms * 8347Hz */
#define MFCC_HOP_SAMPLES    84      /* 10ms * 8347Hz, rounded */
#define MFCC_FFT_SIZE       512     /* next power of 2 */
#define MFCC_FFT_BINS       257     /* N/2+1 */
#define MFCC_NUM_MELS       40
#define MFCC_NUM_COEFFS     13
#define MFCC_MAX_FRAMES      250     /* max frames for long utterances */
#define MFCC_TOTAL_FEATURES (MFCC_MAX_FRAMES * MFCC_NUM_COEFFS)

/* Streaming API (live inference) */
void mfcc_engine_init(void);
void mfcc_engine_feed(const int16_t *samples, int count);
bool mfcc_engine_ready(void);
const float * mfcc_engine_get_features(void);
int  mfcc_engine_get_frame_count(void);  /* actual frames (<= MFCC_MAX_FRAMES) */
void mfcc_engine_reset(void);

/* Batch API (recording post-processing) */
void mfcc_engine_process_all(const int16_t *samples, int total_count);

#endif
