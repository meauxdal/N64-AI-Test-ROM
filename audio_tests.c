#include "audio_tests.h"
#include <libdragon.h>
#include <stdint.h>

// Register access and buffer setup...
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

static int16_t pcm_buffer[8192] __attribute__((aligned(8)));

// --- Restored Original Sequences ---
static test_config_t standard_dc_sweep[] = {
    { 22050, 0x7FFF, 2048, 500 }, // Old Test 1
    { 32000, 0x7FFF, 2048, 500 }, // Old Test 2
    { 44100, 0x7FFF, 2048, 500 }, // Old Test 3
    { 48000, 0x7FFF, 2048, 500 }  // Old Test 4
};

static test_config_t hazardous_test[] = {
    { 44100, 0x8001, 2048, 500 }, // Max Negative DC
    { 48000, 0x0000, 4096, 500 }, // Nyquist Torture (Max Swing)
    { 3000,  0x3FFF, 16,   500 }  // Slow Clock (Safe Minimum)
};

static test_sequence_t sequences[] = {
    {
        "Standard DC Sweep",
        "4 tests: 22k-48k @ 0x7FFF DC",
        standard_dc_sweep,
        4
    },
    {
        "Hazardous Edge Cases",
        "Negative DC, Nyquist, and Slow Clock",
        hazardous_test,
        3
    }
};

// ... calculate_dac_rates and wait_ms_with_abort remain the same ...

int run_single_test(int sequence_id, int test_index) {
    test_sequence_t *seq = &sequences[sequence_id];
    test_config_t *test = &seq->tests[test_index];
    
    // Fill PCM Buffer: Special handling for Nyquist mode
    if (sequence_id == 1 && test->amplitude == 0x0000) {
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
    
    data_cache_hit_writeback(pcm_buffer, test->sample_count * 2);
    IO_WRITE(AI_DRAM_ADDR, PHYS_ADDR(pcm_buffer));
    IO_WRITE(AI_LEN, test->sample_count * 2);
    IO_WRITE(AI_DACRATE, dacrate - 1);
    IO_WRITE(AI_BITRATE, bitrate - 1);
    IO_WRITE(AI_CONTROL, 1);

    while (IO_READ(AI_STATUS) & 0xC0000001);
    
    return wait_ms_with_abort(test->wait_ms);
}