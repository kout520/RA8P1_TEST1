/**
 * MFCC Feature Extractor — Self-contained (no CMSIS-DSP required)
 *
 * Uses standard C FFT + FPU on Cortex-M85 @ 480MHz.
 * ~1.5 MFLOPS sustained → easily real-time.
 */

#include "mfcc_engine.h"
#include "hann_window.h"
#include "mel_filterbank.h"
#include "dct_matrix.h"
#include <string.h>
#include <math.h>

/* ==================================================================
 *                     Static State
 * ================================================================== */

static float g_frame_buffer[MFCC_FRAME_SAMPLES];
static int   g_frame_pos = 0;
static int   g_frame_count = 0;

static float g_mfcc_features[MFCC_TOTAL_FEATURES];
static bool  g_ready = false;

/* FFT + MFCC scratch buffers */
static float g_fft_re[MFCC_FFT_SIZE];
static float g_fft_im[MFCC_FFT_SIZE];
static float g_mag_spectrum[MFCC_FFT_BINS];
static float g_mel_energies[MFCC_NUM_MELS];
static float g_mfcc_frame[MFCC_NUM_COEFFS];

/* ==================================================================
 *                     FFT (Cooley-Tukey DIT, radix-2)
 * ================================================================== */

/* Bit-reversed index lookup */
static int bitrev(int i, int n)
{
    int r = 0;
    for (int b = 1; b < n; b <<= 1) {
        r = (r << 1) | (i & 1);
        i >>= 1;
    }
    return r;
}

/** In-place radix-2 DIT FFT (complex → complex) */
static void fft_c2c(float *re, float *im, int n)
{
    /* Bit-reverse reorder */
    for (int i = 0; i < n; i++) {
        int j = bitrev(i, n);
        if (j > i) {
            float t = re[i]; re[i] = re[j]; re[j] = t;
            t = im[i]; im[i] = im[j]; im[j] = t;
        }
    }

    /* Butterfly loops */
    for (int len = 2; len <= n; len <<= 1) {
        float ang = -2.0f * 3.141592653589793f / len;
        for (int k = 0; k < n; k += len) {
            for (int m = 0; m < len / 2; m++) {
                float w_re = cosf(ang * m);
                float w_im = sinf(ang * m);

                int a = k + m;
                int b = k + m + len / 2;

                float t_re = w_re * re[b] - w_im * im[b];
                float t_im = w_re * im[b] + w_im * re[b];

                re[b] = re[a] - t_re;
                im[b] = im[a] - t_im;
                re[a] = re[a] + t_re;
                im[a] = im[a] + t_im;
            }
        }
    }
}

/* ==================================================================
 *                     MFCC Processing
 * ================================================================== */

/** Process one complete frame (209 samples) → 13 MFCC */
static void process_frame(void)
{
    /* 1. Apply Hann window + copy to FFT buffer (zero-padded to 512) */
    memset(g_fft_re, 0, sizeof(g_fft_re));
    memset(g_fft_im, 0, sizeof(g_fft_im));
    for (int i = 0; i < MFCC_FRAME_SAMPLES; i++) {
        g_fft_re[i] = g_frame_buffer[i] * hann_window[i];
    }

    /* 2. 512-point FFT */
    fft_c2c(g_fft_re, g_fft_im, MFCC_FFT_SIZE);

    /* 3. Magnitude spectrum (first 257 bins) */
    for (int i = 0; i < MFCC_FFT_BINS; i++) {
        g_mag_spectrum[i] = sqrtf(g_fft_re[i] * g_fft_re[i]
                                + g_fft_im[i] * g_fft_im[i]);
    }

    /* 4. Mel filterbank: 40×257 × 257×1 → 40 mel energies */
    for (int m = 0; m < MFCC_NUM_MELS; m++) {
        float sum = 0.0f;
        for (int k = 0; k < MFCC_FFT_BINS; k++) {
            sum += mel_filterbank[m][k] * g_mag_spectrum[k];
        }
        g_mel_energies[m] = sum + 1e-6f;
    }

    /* 5. Log */
    for (int i = 0; i < MFCC_NUM_MELS; i++) {
        g_mel_energies[i] = logf(g_mel_energies[i]);
    }

    /* 6. DCT: 13×40 × 40×1 → 13 MFCC */
    for (int k = 0; k < MFCC_NUM_COEFFS; k++) {
        float sum = 0.0f;
        for (int n = 0; n < MFCC_NUM_MELS; n++) {
            sum += dct_matrix[k][n] * g_mel_energies[n];
        }
        g_mfcc_frame[k] = sum / sqrtf(2.0f * MFCC_NUM_MELS);
    }

    /* 7. Store */
    int offset = g_frame_count * MFCC_NUM_COEFFS;
    memcpy(&g_mfcc_features[offset], g_mfcc_frame,
           MFCC_NUM_COEFFS * sizeof(float));
    g_frame_count++;

    if (g_frame_count >= MFCC_NUM_FRAMES) {
        g_ready = true;
        g_frame_count = 0;
    }
}

/* ==================================================================
 *                     Public API
 * ================================================================== */

void mfcc_engine_init(void)
{
    mfcc_engine_reset();
}

void mfcc_engine_feed(const int16_t *samples, int count)
{
    for (int i = 0; i < count; i++) {
        g_frame_buffer[g_frame_pos] = samples[i] / 32768.0f;
        g_frame_pos++;

        if (g_frame_pos >= MFCC_FRAME_SAMPLES) {
            process_frame();

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

const float * mfcc_engine_get_features(void)
{
    g_ready = false;
    return g_mfcc_features;
}

void mfcc_engine_reset(void)
{
    memset(g_frame_buffer, 0, sizeof(g_frame_buffer));
    memset(g_mfcc_features, 0, sizeof(g_mfcc_features));
    g_frame_pos = 0;
    g_frame_count = 0;
    g_ready = false;
}
