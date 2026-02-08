#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <libdragon.h>
#include "audio_tests.h"

// --- BARE METAL VI REGISTERS ---
#define VI_BASE_REG     0xA4400000
#define VI_STATUS_REG   (VI_BASE_REG + 0x00)
#define VI_ORIGIN_REG   (VI_BASE_REG + 0x04)
#define VI_WIDTH_REG    (VI_BASE_REG + 0x08)
#define VI_INTR_REG     (VI_BASE_REG + 0x0C)
#define VI_CURRENT_REG  (VI_BASE_REG + 0x10)
#define VI_X_SCALE_REG  (VI_BASE_REG + 0x30)
#define VI_Y_SCALE_REG  (VI_BASE_REG + 0x34)

// --- AI REGISTERS ---
#define AI_BASE          0xA4500000
#define AI_STATUS_REG    (AI_BASE + 0x0C)

// --- MANUAL FRAMEBUFFER ---
// 16-bit color: 320x240 pixels * 2 bytes
uint16_t screen_buffer[320 * 240] __attribute__((aligned(64)));

static int menu_selection = 0;
static int running_test = 0;

static void write_reg(uint32_t reg, uint32_t value) {
    *(volatile uint32_t *)reg = value;
}

static uint32_t read_reg(uint32_t reg) {
    return *(volatile uint32_t *)reg;
}

// Manually setup NTSC 320x240 16bpp without the RSP
void vi_init_manual(void) {
    write_reg(VI_STATUS_REG,  0x0000320E); 
    write_reg(VI_WIDTH_REG,   320);
    write_reg(VI_INTR_REG,    0x200);
    write_reg(VI_X_SCALE_REG, 0x00000200);
    write_reg(VI_Y_SCALE_REG, 0x00000400);
    
    // Point VI to our manual buffer (Physical Address)
    uint32_t phys_addr = (uint32_t)screen_buffer & 0x1FFFFFFF;
    write_reg(VI_ORIGIN_REG, phys_addr);
}

void vi_wait_vsync(void) {
    while (read_reg(VI_CURRENT_REG) < 10);
    while (read_reg(VI_CURRENT_REG) > 10);
}

static void draw_menu(int seq_count, const test_sequence_t *sequences) {
    console_clear();
    printf("N64 Audio Test (BARE METAL MODE)\n");
    printf("================================\n\n");
    for (int i = 0; i < seq_count; i++) {
        if (i == menu_selection) printf("> %d. %s\n", i + 1, sequences[i].name);
        else printf("  %d. %s\n", i + 1, sequences[i].name);
    }
}

static void draw_running(int sequence_id) {
    console_clear();
    printf("Running Test #%d\n", sequence_id + 1);
    printf("AI STATUS: 0x%08lX\n", read_reg(AI_STATUS_REG));
    printf("\nPress B to return to menu\n");
}

int main(void) {
    // 1. Initialize manual video & system essentials
    vi_init_manual();
    joypad_init();
    timer_init();
    
    // 2. Setup the surface manually to match modern libdragon struct
    surface_t my_surface = {
        .buffer = screen_buffer,
        .width = 320,
        .height = 240,
        .stride = 320 * 2,
        .flags = 0 // Some versions use flags for format, we'll let console_init handle it
    };
    
    // 3. Initialize console to use our manual surface
    console_init();
    console_set_render_mode(RENDER_MANUAL);

    int seq_count;
    const test_sequence_t *sequences = get_test_sequences(&seq_count);
    
    memset(screen_buffer, 0, sizeof(screen_buffer));

    while (1) {
        joypad_poll();
        joypad_buttons_t keys = joypad_get_buttons_pressed(JOYPAD_PORT_1);

        if (!running_test) {
            if (keys.d_up && menu_selection > 0) menu_selection--;
            if (keys.d_down && menu_selection < seq_count - 1) menu_selection++;
            if (keys.a) {
                running_test = 1;
                run_test_sequence(menu_selection);
            }
            draw_menu(seq_count, sequences);
        } else {
            if (keys.b) running_test = 0;
            draw_running(menu_selection);
        }

        // 4. Render console to our manual buffer
        // Note: Modern console_render usually renders to the current display.
        // If this doesn't show up, we may need to manually draw text.
        console_render(); 

        // 5. Sync the Cache
        data_cache_hit_writeback(screen_buffer, sizeof(screen_buffer));

        // 6. Manual VSync
        vi_wait_vsync();
    }
}