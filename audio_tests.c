#include "audio_tests.h"
#include <libdragon.h>
#include <regsinternal.h>
#include <n64sys.h>
#include <stdint.h>

// Direct MIPS Uncached Address mapping
#define AI_BASE          0xA4500000
#define AI_DRAM_ADDR     (AI_BASE + 0x00)
#define AI_LEN           (AI_BASE + 0x04)
#define AI_CONTROL       (AI_BASE + 0x08)
#define AI_STATUS        (AI_BASE + 0x0C)
#define AI_DACRATE       (AI_BASE + 0x10)
#define AI_BITRATE       (AI_BASE + 0x14)

// Standard IO Accessors
#define IO_READ(addr)       (*(volatile uint32_t*)(addr))
#define IO_WRITE(addr, val) (*(volatile uint32_t*)(addr) = (val))

// Helper for physical addresses (KSEG0 -> Physical)
#define PhysicalAddr(addr) ((uint32_t)(addr) & 0x1FFFFFFF)

#define PHYS_ADDR(x) ((uint32_t)(x) & 0x1FFFFFFF)

#ifndef CLOCKRATE
#define CLOCKRATE 48681812
#endif

static int16_t pcm_buffer[8192] __attribute__((aligned(8)));

void calculate_dac_rates(uint32_t frequency, uint32_t *dacrate, uint32_t *bitrate) {
    *dacrate = (uint32_t)(((2.0 * CLOCKRATE / frequency) + 1) / 2);
    *bitrate = (*dacrate < 66) ? 16 : (*dacrate / 66);
    if (*bitrate > 16) *bitrate = 16;
}

static void wait_ai_busy() {
    while (IO_READ(AI_STATUS) & 0x40000001);
}

static void trigger_audio(int16_t *buffer, uint32_t sample_count, uint32_t dacrate, uint32_t bitrate) {
    data_cache_hit_writeback(buffer, sample_count * sizeof(int16_t));

    IO_WRITE(AI_DACRATE, dacrate - 1);
    IO_WRITE(AI_BITRATE, bitrate - 1);
    
    IO_WRITE(AI_DRAM_ADDR, PhysicalAddr(buffer));
    IO_WRITE(AI_LEN, sample_count * 2);
    IO_WRITE(AI_CONTROL, 1);
}

static test_config_t standard_sweep[] = {
    {22050, 0x7FFF, 16, 1000}, {22050, 0x7FFF, 24, 1000}, {22050, 0x7FFF, 32, 1000},
    {22050, 0x3FFF, 16, 1000}, {22050, 0x3FFF, 24, 1000}, {22050, 0x3FFF, 32, 2000},
    
    {32000, 0x7FFF, 16, 1000}, {32000, 0x7FFF, 24, 1000}, {32000, 0x7FFF, 32, 1000},
    {32000, 0x3FFF, 16, 1000}, {32000, 0x3FFF, 24, 1000}, {32000, 0x3FFF, 32, 2000},
    
    {44100, 0x7FFF, 16, 1000}, {44100, 0x7FFF, 24, 1000}, {44100, 0x7FFF, 32, 1000},
    {44100, 0x3FFF, 16, 1000}, {44100, 0x3FFF, 24, 1000}, {44100, 0x3FFF, 32, 2000},
    
    {48000, 0x7FFF, 16, 1000}, {48000, 0x7FFF, 24, 1000}, {48000, 0x7FFF, 32, 1000},
    {48000, 0x3FFF, 16, 1000}, {48000, 0x3FFF, 24, 1000}, {48000, 0x3FFF, 32, 3000},
};

static test_config_t extended_sweep[] = {
    {22050, 0x7FFF, 8, 1000},  {22050, 0x7FFF, 16, 1000}, {22050, 0x7FFF, 24, 1000},
    {22050, 0x7FFF, 32, 1000}, {22050, 0x7FFF, 48, 1000}, {22050, 0x7FFF, 64, 2000},
    
    {48000, 0x7FFF, 8, 1000},  {48000, 0x7FFF, 16, 1000}, {48000, 0x7FFF, 24, 1000},
    {48000, 0x7FFF, 32, 1000}, {48000, 0x7FFF, 48, 1000}, {48000, 0x7FFF, 64, 2000},
};

static test_config_t quick_test[] = {
    {22050, 0x7FFF, 16, 500}, {32000, 0x7FFF, 16, 500},
    {44100, 0x7FFF, 16, 500}, {48000, 0x7FFF, 16, 500},
};

static test_sequence_t sequences[] = {
    {
        "Standard Sweep",
        "24 tests: 4 freq x 2 amp x 3 lengths",
        standard_sweep,
        sizeof(standard_sweep) / sizeof(test_config_t)
    },
    {
        "Extended Sweep",
        "12 tests: varied sample counts",
        extended_sweep,
        sizeof(extended_sweep) / sizeof(test_config_t)
    },
    {
        "Quick Test",
        "4 tests: all frequencies, 16 samples",
        quick_test,
        sizeof(quick_test) / sizeof(test_config_t)
    }
};

const test_sequence_t* get_test_sequences(int *count) {
    *count = sizeof(sequences) / sizeof(test_sequence_t);
    return sequences;
}

void run_test_sequence(int sequence_id) {
    if (sequence_id < 0 || sequence_id >= (int)(sizeof(sequences) / sizeof(test_sequence_t)))
        return;
    
    test_sequence_t *seq = &sequences[sequence_id];
    
    wait_ms(1000);
    
    for (int i = 0; i < seq->test_count; i++) {
        test_config_t *test = &seq->tests[i];
        
        uint32_t dacrate, bitrate;
        calculate_dac_rates(test->frequency, &dacrate, &bitrate);
        
        for (int j = 0; j < test->sample_count; j++) {
            pcm_buffer[j] = test->amplitude;
        }
        
        trigger_audio(pcm_buffer, test->sample_count, dacrate, bitrate);
        wait_ai_busy();
        wait_ms(test->wait_ms);
    }
}
