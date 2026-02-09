#include "audio_tests.h"
#include <libdragon.h>
#include <regsinternal.h>
#include <n64sys.h>
#include <stdint.h>

// Registers & Signals used:
// AI_DACRATE: Audio Interface DAC Rate - Controls output frequency
// AI_BITRATE: Audio Interface Bit Rate - Controls bitrate divider
// AI_STATUS:  Audio Interface Status - Used to check if FIFO is busy
// AI_CONTROL: Audio Interface Control - Used to enable audio DMA

#define AI_BASE          0xA4500000
#define AI_DRAM_ADDR     (AI_BASE + 0x00)
#define AI_LEN           (AI_BASE + 0x04)
#define AI_CONTROL       (AI_BASE + 0x08)
#define AI_STATUS        (AI_BASE + 0x0C)
#define AI_DACRATE       (AI_BASE + 0x10)
#define AI_BITRATE       (AI_BASE + 0x14)

#define IO_READ(addr)       (*(volatile uint32_t*)(addr))
#define IO_WRITE(addr, val) (*(volatile uint32_t*)(addr) = (val))
#define PHYS_ADDR(x)        ((uint32_t)(x) & 0x1FFFFFFF)

static int16_t pcm_buffer[8192] __attribute__((aligned(8)));

// ... (calculate_dac_rates and trigger_audio stay the same) ...

// Removed 'static' so main.c can call this directly
int wait_ms_with_abort(uint32_t ms) {
    uint32_t start = timer_ticks();
    uint32_t wait_ticks = TICKS_FROM_MS(ms);
    
    while (timer_ticks() - start < wait_ticks) {
        joypad_poll();
        joypad_buttons_t keys = joypad_get_buttons_pressed(JOYPAD_PORT_1);
        if (keys.b) return 1; // Abort signal
    }
    return 0;
}

// Changed to return int to signal the abort back to main loop
int run_single_test(int sequence_id, int test_index) {
    test_sequence_t *seq = (test_sequence_t*)&get_test_sequences(NULL)[sequence_id];
    test_config_t *test = &seq->tests[test_index];
    
    uint32_t dacrate, bitrate;
    calculate_dac_rates(test->frequency, &dacrate, &bitrate);
    
    for (int j = 0; j < test->sample_count; j++) {
        pcm_buffer[j] = test->amplitude;
    }
    
    // trigger_audio logic here...
    data_cache_hit_writeback(pcm_buffer, test->sample_count * 2);
    IO_WRITE(AI_DACRATE, dacrate - 1);
    IO_WRITE(AI_BITRATE, bitrate - 1);
    IO_WRITE(AI_DRAM_ADDR, PHYS_ADDR(pcm_buffer));
    IO_WRITE(AI_LEN, test->sample_count * 2);
    IO_WRITE(AI_CONTROL, 1);

    // Wait for the busy bit to clear
    while (IO_READ(AI_STATUS) & 0xC0000001);
    
    // Return the abort status from the post-test wait
    return wait_ms_with_abort(test->wait_ms);
}