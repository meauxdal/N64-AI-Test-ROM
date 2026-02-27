#ifndef AUDIO_TESTS_H
#define AUDIO_TESTS_H

#include <stdint.h>

#define CLOCKRATE           48681818
#define MAX_TESTS           32
#define AUDIO_LEN           0x800       // 2048 bytes = ~11.6ms at 44100Hz
#define EXPECTED_IRQ_COUNT  8
#define TIMEOUT_MS          500

// --- Existing test sequence types ---

typedef struct {
    uint32_t frequency;
    uint16_t amplitude;
    uint16_t sample_count;
    uint32_t wait_ms;
} test_config_t;

typedef struct {
    const char *name;
    const char *description;
    test_config_t *tests;
    int test_count;
} test_sequence_t;

// --- IRQ test result ---

typedef struct {
    uint32_t expected;
    uint32_t actual;
    int pass;       // 1 = pass, 0 = fail
    int timed_out;  // 1 = chain stalled before expected_count
} irq_test_result_t;

// --- Existing sequence API ---
int run_single_test(int sequence_id, int test_index);
int wait_ms_with_abort(uint32_t ms);
const test_sequence_t* get_test_sequences(int *count);
void calculate_dac_rates(uint32_t frequency, uint32_t *dacrate, uint32_t *bitrate);

// --- IRQ test API ---
irq_test_result_t run_irq_test(void);

#endif
