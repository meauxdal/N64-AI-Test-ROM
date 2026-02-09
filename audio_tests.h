#ifndef AUDIO_TESTS_H
#define AUDIO_TESTS_H

#include <stdint.h>

#define CLOCKRATE 48681818
#define MAX_TESTS 32

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

// Returns: 1 if user aborted (Pressed B), 0 if completed normally
int run_single_test(int sequence_id, int test_index);

// Performs a wait that can be interrupted by the B button
int wait_ms_with_abort(uint32_t ms);

const test_sequence_t* get_test_sequences(int *count);
void calculate_dac_rates(uint32_t frequency, uint32_t *dacrate, uint32_t *bitrate);

#endif