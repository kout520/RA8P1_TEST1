/**
 * MRAM persistent storage for speaker data.
 *
 * RA8P1 MRAM at 0x02000000 (1MB): memory-mapped, survives power cycles.
 * Writes need FACI (Flash Access Circuit Interface) unlock → write → lock.
 * Write unit: 32 bytes, must be 32-byte aligned.
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "hal_data.h"

/* ── RA8P1 MRAM write protection register (R_SYSTEM) ────────────── */
/* R_SYSTEM base: 0x4001E000, FWEPROR offset: 0x416 */
#define R_SYSTEM_FWEPROR  (*(volatile uint8_t *)(0x4001E416u))
#define FWEPROR_UNLOCK    0x01u   /* FLWE=01: disable write protection */
#define FWEPROR_LOCK      0x02u   /* FLWE=10: enable write protection  */

/* ── Linker-placed MRAM page (.flash_noinit, NOT zeroed at boot) ── */
#define MRAM_SIZE         4096u
#define MRAM_MAGIC        0x4D52414Du   /* "MRAM" */
#define MRAM_VERSION      1
#define MRAM_WRITE_SIZE   32u            /* MRAM write unit: 32 bytes */

/* Linker places this in MRAM — survives power cycles */
static uint8_t mram_page[MRAM_SIZE]
    __attribute__((section(".flash_noinit"), aligned(32)));

/* ── MRAM data layout ───────────────────────────────────────────── */
typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t data_size;
    uint32_t checksum;        /* XOR of payload */
    /* payload follows at offset MRAM_WRITE_SIZE */
} mram_header_t;


/* ── Low-level MRAM write (32-byte aligned unit) ────────────────── */

static void mram_write32(const uint8_t *src, uint32_t offset)
{
    volatile uint32_t *dst = (volatile uint32_t *)(mram_page + offset);

    /* Copy 32 bytes = 8 × uint32_t */
    for (int i = 0; i < 8; i++) {
        dst[i] = ((uint32_t)src[i*4+0])
               | ((uint32_t)src[i*4+1] << 8)
               | ((uint32_t)src[i*4+2] << 16)
               | ((uint32_t)src[i*4+3] << 24);
    }
}

/* ── Public API ─────────────────────────────────────────────────── */

/**
 * Save to MRAM — FACI unlock → write → lock.
 * Writes header (32B) + payload in 32-byte aligned chunks.
 */
bool mram_save(const uint8_t *data, uint32_t len)
{
    if (len == 0 || len > (MRAM_SIZE - MRAM_WRITE_SIZE))
        return false;

    /* 1. Unlock MRAM: disable write protection (FLWE=01) */
    R_SYSTEM_FWEPROR = FWEPROR_UNLOCK;

    /* 2. Build header (32 bytes) */
    uint8_t hdr_buf[MRAM_WRITE_SIZE];
    memset(hdr_buf, 0xFF, sizeof(hdr_buf));  /* fill erased state */
    mram_header_t hdr;
    hdr.magic     = MRAM_MAGIC;
    hdr.version   = MRAM_VERSION;
    hdr.data_size = len;
    hdr.checksum  = 0;
    for (uint32_t i = 0; i < len; i++) hdr.checksum ^= data[i];
    memcpy(hdr_buf, &hdr, sizeof(hdr));

    /* 3. Write header */
    mram_write32(hdr_buf, 0);

    /* 4. Write payload (32-byte aligned chunks, pad with 0xFF) */
    uint8_t  chunk[MRAM_WRITE_SIZE];
    uint32_t pos = 0;
    while (pos < len) {
        uint32_t rem = len - pos;
        uint32_t chunk_len = (rem >= MRAM_WRITE_SIZE) ? MRAM_WRITE_SIZE : rem;
        memset(chunk, 0xFF, sizeof(chunk));
        memcpy(chunk, data + pos, chunk_len);
        mram_write32(chunk, MRAM_WRITE_SIZE + pos);
        pos += MRAM_WRITE_SIZE;
    }

    /* 5. Re-enable write protection (FLWE=10) */
    R_SYSTEM_FWEPROR = FWEPROR_LOCK;

    printf("MRAM: saved %lu bytes\r\n", (unsigned long)len);
    return true;
}

/**
 * Load from MRAM.
 * Returns data length, or -1 if no valid data.
 */
int32_t mram_load(uint8_t *buf, uint32_t max_len)
{
    mram_header_t *hdr = (mram_header_t *)mram_page;

    if (hdr->magic != MRAM_MAGIC)
        return -1;
    if (hdr->version != MRAM_VERSION)
        return -1;
    if (hdr->data_size > max_len)
        return -1;

    const uint8_t *payload = (const uint8_t *)(mram_page + MRAM_WRITE_SIZE);
    memcpy(buf, payload, hdr->data_size);

    /* Verify checksum */
    uint32_t cs = 0;
    for (uint32_t i = 0; i < hdr->data_size; i++) cs ^= buf[i];
    if (cs != hdr->checksum)
        return -1;

    printf("MRAM: loaded %lu bytes\r\n", (unsigned long)hdr->data_size);
    return (int32_t)hdr->data_size;
}

/**
 * Erase MRAM: invalidate magic so next load returns -1.
 */
void mram_erase(void)
{
    uint8_t zero_hdr[MRAM_WRITE_SIZE];
    memset(zero_hdr, 0, sizeof(zero_hdr));

    R_SYSTEM_FWEPROR = FWEPROR_UNLOCK;
    mram_write32(zero_hdr, 0);
    R_SYSTEM_FWEPROR = FWEPROR_LOCK;

    printf("MRAM: erased\r\n");
}
