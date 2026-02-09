#include "audio_tests.h"
#include <libdragon.h>
#include <stdint.h>

// AI Register Definitions
#define AI_BASE          0xA4500000
#define AI_DRAM_ADDR     (AI_BASE + 0x00)
#define AI_LEN           (AI_BASE + 0x04)
#define AI_CONTROL       (AI_BASE + 0x08)
#define AI_STATUS        (AI_BASE + 0x0C)
#define AI_DACRATE       (AI_BASE + 0x10)
#define AI_BITRATE       (AI_BASE + 0x14)

#define IO_WRITE(addr, val) (*(volatile uint32_t*)(addr) = (val))
#define IO_READ(addr)       (*(volatile uint32_t*)(addr))
#define PHYS_ADDR(x)        ((uint32_t)(x) & 0x1FFFFFFF)

// Buffer aligned for AI DMA
static int16_t pcm_buffer[8192] __attribute__((aligned(8)));

// --- Test Sequences ---

// 1. DC Offset Sweep (Current Standard: 2048 samples)
static test_config_t standard_dc_sweep[] = {
    { 22050, 0x7FFF, 2048, 1000 },
    { 32000, 0x7FFF, 2048, 1000 },
    { 44100, 0x7FFF, 2048, 1000 },
    { 48000, 0x7FFF, 2048, 1000 }
};

// 2. Edge Cases
static test_config_t edge_cases[] = {
    { 44100, 0x8001, 2048, 1000 }, // Max Negative DC
    { 48000, 0x0000, 4096, 1000 }, // Nyquist Torture (Max Swing)
    { 3000,  0x3FFF, 16,   1000 }  // Slow Clock Stress
};

// 3. Legacy V5 Sweep (Matches EXACT v5/v6 behavior: 8176 bytes / 4088 samples)
static test_config_t legacy_v5_sweep[] = {
    { 22050, 0x7FFF, 4088, 1000 },
    { 32000, 0x7FFF, 4088, 1000 },
    { 44100, 0x7FFF, 4088, 1000 },
    { 48000, 0x7FFF, 4088, 1000 }
};

static test_sequence_t sequences[] = {
    {
        "DC Offset Sweep",
        "22.05, 32, 44.1, 48KHz @ 0x7FFF",
        standard_dc_sweep,
        4
    },
    {
        "Edge Cases",
        "Negative DC, Nyquist, Slow Clock",
        edge_cases,
        3
    },
    {
        "Legacy Sweep",
        "8176 byte payload from V5",
        legacy_v5_sweep,
        4
    }
};

// --- Logic ---

const test_sequence_t* get_test_sequences(int *count) {
    if (count) *count = sizeof(sequences) / sizeof(test_sequence_t);
    return sequences;
}

void calculate_dac_rates(uint32_t frequency, uint32_t *dacrate, uint32_t *bitrate) {
    *dacrate = (uint32_t)(((2.0 * CLOCKRATE / frequency) + 1) / 2);
    *bitrate = (*dacrate < 66) ? 16 : (*dacrate / 66);
    if (*bitrate > 16) *bitrate = 16;
}

int wait_ms_with_abort(uint32_t ms) {
    uint32_t start = timer_ticks();
    uint32_t wait_ticks = TICKS_FROM_MS(ms);
    while (timer_ticks() - start < wait_ticks) {
        joypad_poll();
        joypad_buttons_t keys = joypad_get_buttons_pressed(JOYPAD_PORT_1);
        if (keys.b) return 1; 
    }
    return 0;
}

int run_single_test(int sequence_id, int test_index) {
    test_sequence_t *seq = &sequences[sequence_id];
    test_config_t *test = &seq->tests[test_index];
    
    // Fill Buffer
    if (sequence_id == 1 && test->amplitude == 0x0000) {
        // Nyquist Torture: Full Swing + / -
        for (int j = 0; j < test->sample_count; j++) {
            pcm_buffer[j] = (j % 2 == 0) ? 0x7FFF : 0x8001;
        }
    } else {
        for (int j = 0; j < test->sample_count; j++) {
            pcm_buffer[j] = (int16_t)test->amplitude;
        }
    }

    uint32_t dacrate, bitrate;
    calculate_dac_rates(test->frequency, &dacrate, &bitrate);
    
    // Hardware Trigger
    data_cache_hit_writeback(pcm_buffer, test->sample_count * 2);
    IO_WRITE(AI_DRAM_ADDR, PHYS_ADDR(pcm_buffer));
    IO_WRITE(AI_LEN, test->sample_count * 2); // Byte length
    IO_WRITE(AI_DACRATE, dacrate - 1);
    IO_WRITE(AI_BITRATE, bitrate - 1);
    IO_WRITE(AI_CONTROL, 1);

    // Wait for FIFO to drain
    while (IO_READ(AI_STATUS) & 0xC0000001);
    
    return wait_ms_with_abort(test->wait_ms);
}