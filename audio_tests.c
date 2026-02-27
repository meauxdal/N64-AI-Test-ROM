#include "audio_tests.h"
#include <libdragon.h>
#include <stdint.h>

// --- AI Register Definitions ---
#define AI_BASE         0xA4500000
#define AI_DRAM_ADDR    (AI_BASE + 0x00)
#define AI_LEN          (AI_BASE + 0x04)
#define AI_CONTROL      (AI_BASE + 0x08)
#define AI_STATUS       (AI_BASE + 0x0C)
#define AI_DACRATE      (AI_BASE + 0x10)
#define AI_BITRATE      (AI_BASE + 0x14)

// --- MI Register Definitions ---
#define MI_INTR_REG         0xA4300008  // read: which IRQs are pending
#define MI_INTR_MASK_REG    0xA430000C  // write: set/clear IRQ masks
#define MI_AI_INTR          (1 << 2)    // bit 2 = AI interrupt pending
#define MI_MASK_SET_AI      (1 << 5)    // bit 5 = enable AI IRQ
#define MI_MASK_CLR_AI      (1 << 4)    // bit 4 = disable AI IRQ

#define IO_WRITE(addr, val) (*(volatile uint32_t*)(addr) = (val))
#define IO_READ(addr)       (*(volatile uint32_t*)(addr))
#define PHYS_ADDR(x)        ((uint32_t)(x) & 0x1FFFFFFF)

// --- PCM buffer — aligned for AI DMA ---
static int16_t pcm_buffer[AUDIO_LEN / 2] __attribute__((aligned(8)));

// --- IRQ state — volatile: modified in interrupt context, read in main ---
static volatile uint32_t irq_count    = 0;
static volatile uint32_t expected_count = 0;

// -------------------------------------------------------------------------
// raw_ai_trigger
// Atomic bare-metal AI DMA trigger. Correct write order enforced.
// +8 offset applied explicitly to prevent AI address-carry silicon bug.
// -------------------------------------------------------------------------
void raw_ai_trigger(void *pcm_buf, uint32_t length, uint32_t target_freq) {
    uint32_t dacrate = (((2 * CLOCKRATE / target_freq) + 1) / 2) - 1;
    uint32_t bitrate_calc = (dacrate + 1) / 66;
    uint32_t bitrate = (bitrate_calc > 16 ? 16 : bitrate_calc) - 1;

    uint32_t phys_addr = PHYS_ADDR(pcm_buf) + 8;

    IO_WRITE(AI_CONTROL,  0);           // 1. disable DAC
    IO_WRITE(AI_DACRATE,  dacrate);     // 2. clock — must precede AI_LEN
    IO_WRITE(AI_BITRATE,  bitrate);     // 3. bit width — must precede AI_LEN
    IO_WRITE(AI_DRAM_ADDR, phys_addr);  // 4. address
    IO_WRITE(AI_LEN,      length);      // 5. TRIGGER — edge-triggered DMA start
    IO_WRITE(AI_CONTROL,  1);           // 6. enable DAC
}

// -------------------------------------------------------------------------
// ai_irq_handler
// Called by libdragon exception dispatch on any interrupt.
// Guards on MI_INTR_REG to confirm this is an AI IRQ — VI and timer
// interrupts are also live from display_init/timer_init.
// -------------------------------------------------------------------------
static void ai_irq_handler(exception_t *ex) {
    // Confirm this is actually an AI IRQ
    if (!(IO_READ(MI_INTR_REG) & MI_AI_INTR)) return;

    // Clear AI IRQ — any write to AI_STATUS clears it on hardware
    IO_WRITE(AI_STATUS, 0);

    // Count it
    irq_count++;

    // Keep the chain going if we haven't hit the target
    if (irq_count < expected_count) {
        raw_ai_trigger(pcm_buffer, AUDIO_LEN, 44100);
    }
    // Otherwise: chain stops. Main loop sees irq_count == expected_count.
}

// -------------------------------------------------------------------------
// irq_test_init / irq_test_teardown
// -------------------------------------------------------------------------
static void irq_test_init(uint32_t count) {
    irq_count       = 0;
    expected_count  = count;

    register_exception_handler(ai_irq_handler);
    IO_WRITE(MI_INTR_MASK_REG, MI_MASK_SET_AI);
}

static void irq_test_teardown(void) {
    IO_WRITE(MI_INTR_MASK_REG, MI_MASK_CLR_AI);
    register_exception_handler(NULL);
}

// -------------------------------------------------------------------------
// run_irq_test
// Fills buffer, starts chain, spin-waits for expected IRQ count or timeout.
// Returns result struct with expected, actual, pass, timed_out.
// -------------------------------------------------------------------------
irq_test_result_t run_irq_test(void) {
    irq_test_result_t result = {0};
    result.expected = EXPECTED_IRQ_COUNT;

    // Fill buffer — DC plateau, consistent with bare metal baseline
    for (int i = 0; i < AUDIO_LEN / 2; i++) {
        pcm_buffer[i] = 0x7FFF;
    }
    data_cache_hit_writeback(pcm_buffer, AUDIO_LEN);

    // Init — registers handler, enables AI IRQ in MI, enables interrupts
    irq_test_init(EXPECTED_IRQ_COUNT);

    // Push first buffer — chain starts here
    raw_ai_trigger(pcm_buffer, AUDIO_LEN, 44100);

    // Spin-wait for chain to complete or timeout
    uint32_t start = timer_ticks();
    uint32_t timeout_ticks = TICKS_FROM_MS(TIMEOUT_MS);

    while (irq_count < EXPECTED_IRQ_COUNT) {
        if (timer_ticks() - start > timeout_ticks) {
            result.timed_out = 1;
            break;
        }
    }

    // Teardown — disable AI IRQ, restore default handler
    irq_test_teardown();

    result.actual = irq_count;
    result.pass   = (irq_count == EXPECTED_IRQ_COUNT && !result.timed_out);

    return result;
}

// -------------------------------------------------------------------------
// draw_irq_result
// Displays result on screen. Called from main after run_irq_test returns.
// -------------------------------------------------------------------------
void draw_irq_result(surface_t *disp, irq_test_result_t *r) {
    graphics_fill_screen(disp, 0);

    // Header
    graphics_set_color(graphics_make_color(255, 255, 255, 255), 0);
    graphics_draw_text(disp, 25, 20, "AI IRQ Timing Test");
    graphics_draw_text(disp, 25, 32, "------------------");

    // Expected / Actual counts
    char exp_txt[32], act_txt[32];
    snprintf(exp_txt, sizeof(exp_txt), "Expected IRQs: %lu", (unsigned long)r->expected);
    snprintf(act_txt, sizeof(act_txt), "Actual IRQs:   %lu", (unsigned long)r->actual);

    graphics_set_color(graphics_make_color(200, 200, 200, 255), 0);
    graphics_draw_text(disp, 25, 70, exp_txt);
    graphics_draw_text(disp, 25, 85, act_txt);

    // Timeout note — amber, distinguishes stalled chain from wrong count
    if (r->timed_out) {
        graphics_set_color(graphics_make_color(255, 180, 0, 255), 0);
        graphics_draw_text(disp, 25, 105, "Chain stalled (timeout)");
    }

    // PASS / FAIL
    if (r->pass) {
        graphics_set_color(graphics_make_color(0, 255, 0, 255), 0);
        graphics_draw_text(disp, 25, 140, "PASS");
    } else {
        graphics_set_color(graphics_make_color(255, 0, 0, 255), 0);
        graphics_draw_text(disp, 25, 140, "FAIL");
    }

    graphics_set_color(graphics_make_color(150, 150, 150, 255), 0);
    graphics_draw_text(disp, 25, 210, "A: Rerun  B: Menu");
}

// -------------------------------------------------------------------------
// Existing test sequence infrastructure — unchanged from v7
// -------------------------------------------------------------------------

static test_config_t standard_dc_sweep[] = {
    { 22050, 0x7FFF, 2048, 1000 },
    { 32000, 0x7FFF, 2048, 1000 },
    { 44100, 0x7FFF, 2048, 1000 },
    { 48000, 0x7FFF, 2048, 1000 }
};

static test_config_t edge_cases[] = {
    { 44100, 0x8001, 2048, 1000 },
    { 48000, 0x0000, 4096, 1000 },
    { 3000,  0x3FFF, 16,   1000 }
};

static test_config_t legacy_v5_sweep[] = {
    { 22050, 0x7FFF, 4088, 1000 },
    { 32000, 0x7FFF, 4088, 1000 },
    { 44100, 0x7FFF, 4088, 1000 },
    { 48000, 0x7FFF, 4088, 1000 }
};

static test_sequence_t sequences[] = {
    { "DC Offset Sweep",  "22.05, 32, 44.1, 48KHz @ 0x7FFF", standard_dc_sweep, 4 },
    { "Edge Cases",       "Negative DC, Nyquist, Slow Clock",  edge_cases,        3 },
    { "Legacy Sweep",     "8176 byte payload from V5",          legacy_v5_sweep,   4 }
};

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
