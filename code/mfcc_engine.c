/**
 * MFCC Feature Extractor — Variable-length utterances (VAD-based)
 *
 * Supports up to 250 frames (~2.5s of speech at 10ms hop).
 * FFT: hand-written radix-2 DIT, no CMSIS-DSP dependency.
 */

#include "mfcc_engine.h"
#include "hann_window.h"
#include "mel_filterbank.h"
#include "dct_matrix.h"
#include <arm_mve.h>
#include <string.h>
#include <math.h>

/* ==================================================================
 *                     Static State
 * ================================================================== */

static float g_frame_buffer[MFCC_FRAME_SAMPLES];
static int   g_frame_pos = 0;
static int   g_frame_count = 0;    /* actual frames collected */

static float g_mfcc_features[MFCC_TOTAL_FEATURES];  /* up to 250 frames */
static bool  g_ready = false;

/* FFT + MFCC scratch buffers */
static float g_fft_re[MFCC_FFT_SIZE];
static float g_fft_im[MFCC_FFT_SIZE];
static float g_mag_spectrum[MFCC_FFT_BINS];
static float g_mel_energies[MFCC_NUM_MELS];
static float g_mfcc_frame[MFCC_NUM_COEFFS];

#define MFCC_MIN_FRAMES  15   /* minimum frames for valid utterance (~150ms) */
#define PREEMPH_ALPHA    0.97f  /* pre-emphasis coefficient */
static float preemph_prev = 0.0f;  /* cross-buffer pre-emphasis state */

/* ==================================================================
 *          FFT (Cooley-Tukey DIT, radix-2, twiddle table)
 * ================================================================== */

/* Pre-computed twiddle factors for 512-pt FFT: W_N^k = exp(-j*2π*k/N)
   Total: 1+2+4+8+16+32+64+128+256 = 511 entries (FFT_N - 1) */
#define FFT_N 512
static float g_twiddle_re[FFT_N - 1];
static float g_twiddle_im[FFT_N - 1];
static bool  g_twiddle_ready = false;

static void twiddle_init(void)
{
    if (g_twiddle_ready) return;
    int idx = 0;
    for (int len = 2; len <= FFT_N; len <<= 1) {
        float ang = -2.0f * 3.141592653589793f / (float)len;
        for (int m = 0; m < len / 2; m++) {
            g_twiddle_re[idx] = cosf(ang * (float)m);
            g_twiddle_im[idx] = sinf(ang * (float)m);
            idx++;
        }
    }
    g_twiddle_ready = true;
}

static int bitrev(int i, int n)
{
    int r = 0;
    for (int b = 1; b < n; b <<= 1) {
        r = (r << 1) | (i & 1);
        i >>= 1;
    }
    return r;
}

static void fft_c2c(float *__restrict re, float *__restrict im, int n)
{
    /* Bit-reversal permutation */
    for (int i = 0; i < n; i++) {
        int j = bitrev(i, n);
        if (j > i) {
            float t = re[i]; re[i] = re[j]; re[j] = t;
            t = im[i]; im[i] = im[j]; im[j] = t;
        }
    }
    /* Butterfly stages with pre-computed twiddles */
    int tidx = 0;
    for (int len = 2; len <= n; len <<= 1) {
        int half = len / 2;
        for (int k = 0; k < n; k += len) {
            for (int m = 0; m < half; m++) {
                float w_re = g_twiddle_re[tidx + m];
                float w_im = g_twiddle_im[tidx + m];
                int a = k + m;
                int b = k + m + half;
                float t_re = w_re * re[b] - w_im * im[b];
                float t_im = w_re * im[b] + w_im * re[b];
                re[b] = re[a] - t_re;
                im[b] = im[a] - t_im;
                re[a] = re[a] + t_re;
                im[a] = im[a] + t_im;
            }
        }
        tidx += half;
    }
}

/* ==================================================================
 *                     MFCC Processing
 * ================================================================== */

static void process_frame(void)
{
    /* 1. Pre-emphasis + Hann window + zero-pad (MVE: 4 floats/iter) */
    memset(g_fft_re, 0, sizeof(g_fft_re));
    memset(g_fft_im, 0, sizeof(g_fft_im));
    int i;
    for (i = 0; i <= MFCC_FRAME_SAMPLES - 4; i += 4) {
        float32x4_t samp = vld1q(&g_frame_buffer[i]);
        /* build [preemph_prev, samp0, samp1, samp2] safely via array */
        float pv[4];
        pv[0] = preemph_prev;
        pv[1] = vgetq_lane(samp, 0);
        pv[2] = vgetq_lane(samp, 1);
        pv[3] = vgetq_lane(samp, 2);
        float32x4_t prev_v = vld1q(pv);
        float32x4_t alpha  = vdupq_n_f32(PREEMPH_ALPHA);
        float32x4_t pre    = vsubq(samp, vmulq(alpha, prev_v));
        float32x4_t hann   = vld1q(&hann_window[i]);
        vst1q(&g_fft_re[i], vmulq(pre, hann));
        preemph_prev = vgetq_lane(samp, 3);
    }
    /* Tail: < 4 remaining samples */
    for (; i < MFCC_FRAME_SAMPLES; i++) {
        float sample = g_frame_buffer[i];
        float pre = sample - PREEMPH_ALPHA * preemph_prev;
        preemph_prev = sample;
        g_fft_re[i] = pre * hann_window[i];
    }

    /* 2. 512-point FFT */
    fft_c2c(g_fft_re, g_fft_im, MFCC_FFT_SIZE);

    /* 3. Magnitude spectrum */
    for (int i = 0; i < MFCC_FFT_BINS; i++) {
        g_mag_spectrum[i] = sqrtf(g_fft_re[i] * g_fft_re[i]
                                + g_fft_im[i] * g_fft_im[i]);
    }

    /* 4. Mel filterbank (MVE: 4-wide dot product per filter) */
    for (int m = 0; m < MFCC_NUM_MELS; m++) {
        float32x4_t sum4 = vdupq_n_f32(0.0f);
        int k;
        for (k = 0; k <= MFCC_FFT_BINS - 4; k += 4) {
            sum4 = vfmaq(sum4,
                vld1q(&mel_filterbank[m][k]),
                vld1q(&g_mag_spectrum[k]));
        }
        float sum = vgetq_lane(sum4, 0) + vgetq_lane(sum4, 1)
                  + vgetq_lane(sum4, 2) + vgetq_lane(sum4, 3);
        for (; k < MFCC_FFT_BINS; k++) {
            sum += mel_filterbank[m][k] * g_mag_spectrum[k];
        }
        g_mel_energies[m] = sum + 1e-6f;
    }

    /* 5. Log */
    for (int i = 0; i < MFCC_NUM_MELS; i++) {
        g_mel_energies[i] = logf(g_mel_energies[i]);
    }

    /* 6. DCT (MVE: 4-wide dot per coefficient) */
    {
        float inv_n = 1.0f / sqrtf(2.0f * MFCC_NUM_MELS);
        for (int k = 0; k < MFCC_NUM_COEFFS; k++) {
            float32x4_t sum4 = vdupq_n_f32(0.0f);
            int n;
            for (n = 0; n <= MFCC_NUM_MELS - 4; n += 4) {
                sum4 = vfmaq(sum4,
                    vld1q(&dct_matrix[k][n]),
                    vld1q(&g_mel_energies[n]));
            }
            float s = vgetq_lane(sum4, 0) + vgetq_lane(sum4, 1)
                    + vgetq_lane(sum4, 2) + vgetq_lane(sum4, 3);
            for (; n < MFCC_NUM_MELS; n++) s += dct_matrix[k][n] * g_mel_energies[n];
            g_mfcc_frame[k] = s * inv_n;
        }
    }

    /* 7. Cepstral liftering (sinusoidal, L=22) */
    #define LIFTER_L 22.0f
    g_mfcc_frame[0] *= 1.0f;  /* C0: no lifter */
    for (int k = 1; k < MFCC_NUM_COEFFS; k++) {
        float lifter = 1.0f + (LIFTER_L / 2.0f) * sinf(3.14159265f * k / LIFTER_L);
        g_mfcc_frame[k] *= lifter;
    }

    /* 8. Store frame (up to max) */
    if (g_frame_count < MFCC_MAX_FRAMES) {
        int offset = g_frame_count * MFCC_NUM_COEFFS;
        memcpy(&g_mfcc_features[offset], g_mfcc_frame,
               MFCC_NUM_COEFFS * sizeof(float));
        g_frame_count++;
    }

    /* Ready if we have meaningful speech */
    if (g_frame_count >= MFCC_MIN_FRAMES) {
        g_ready = true;
    }
}

/* ==================================================================
 *                     Public API
 * ================================================================== */

void mfcc_engine_init(void)
{
    twiddle_init();
    mfcc_engine_reset();
}

void mfcc_engine_feed(const int16_t *samples, int count)
{
    if (g_frame_count >= MFCC_MAX_FRAMES) return;  /* capped */

    for (int i = 0; i < count; i++) {
        g_frame_buffer[g_frame_pos] = samples[i] / 32768.0f;
        g_frame_pos++;

        if (g_frame_pos >= MFCC_FRAME_SAMPLES) {
            process_frame();
            if (g_frame_count >= MFCC_MAX_FRAMES) {
                g_frame_pos = 0;
                return;  /* stop, buffer full */
            }
            int overlap = MFCC_FRAME_SAMPLES - MFCC_HOP_SAMPLES;
            memmove(g_frame_buffer, &g_frame_buffer[MFCC_HOP_SAMPLES],
                    overlap * sizeof(float));
            g_frame_pos = overlap;
        }
    }
}

bool mfcc_engine_ready(void)
{
    return g_ready;
}

int mfcc_engine_get_frame_count(void)
{
    return g_frame_count;
}

const float * mfcc_engine_get_features(void)
{
    g_ready = false;
    return g_mfcc_features;
}

void mfcc_engine_reset(void)
{
    memset(g_frame_buffer, 0, sizeof(g_frame_buffer));
    g_frame_pos = 0;
    g_frame_count = 0;
    g_ready = false;
    preemph_prev = 0.0f;  /* reset pre-emphasis state */
    /* Don't clear g_mfcc_features (caller reads after reset) */
}

/**
 * Batch process all samples at once (for recording post-processing).
 * After this call, use mfcc_engine_get_frame_count() and get_features().
 */
void mfcc_engine_process_all(const int16_t *samples, int total_count)
{
    mfcc_engine_reset();
    for (int pos = 0; pos < total_count; ) {
        int chunk = 512;
        if (pos + chunk > total_count) chunk = total_count - pos;
        mfcc_engine_feed(&samples[pos], chunk);
        pos += chunk;
        if (g_frame_count >= MFCC_MAX_FRAMES) break;
    }
}
