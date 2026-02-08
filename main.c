#include <stdio.h>
#include <stdint.h>
#include <libdragon.h>
#include "audio_tests.h"

// --- BARE METAL VI REGISTERS ---
#define VI_BASE_REG     0xA4400000
#define VI_STATUS_REG   (VI_BASE_REG + 0x00)
#define VI_ORIGIN_REG   (VI_BASE_REG + 0x04)
#define VI_WIDTH_REG    (VI_BASE_REG + 0x08)
#define VI_INTR_REG     (VI_BASE_REG + 0x0C)
#define VI_CURRENT_REG  (VI_BASE_REG + 0x10)
#define VI_BURST_REG    (VI_BASE_REG + 0x14)
#define VI_V_SYNC_REG   (VI_BASE_REG + 0x18)
#define VI_H_SYNC_REG   (VI_BASE_REG + 0x1C)
#define VI_LEAP_REG     (VI_BASE_REG + 0x20)
#define VI_H_START_REG  (VI_BASE_REG + 0x24)
#define VI_V_START_REG  (VI_BASE_REG + 0x28)
#define VI_V_BURST_REG  (VI_BASE_REG + 0x2C)
#define VI_X_SCALE_REG  (VI_BASE_REG + 0x30)
#define VI_Y_SCALE_REG  (VI_BASE_REG + 0x34)

// --- AI REGISTERS ---
#define AI_BASE          0xA4500000
#define AI_CONTROL_REG   (AI_BASE + 0x08)
#define AI_STATUS_REG    (AI_BASE + 0x0C)
#define AI_DACRATE_REG   (AI_BASE + 0x10)

// --- MANUAL FRAMEBUFFER ---
// 320 pixels * 240 lines * 2 bytes (16-bit color)
// Aligned to 64 bytes for cache safety
uint16_t screen_buffer[320 * 240] __attribute__((aligned(64)));

static int menu_selection = 0;
static int running_test = 0;

// Helper to write to hardware registers
static void write_reg(uint32_t reg, uint32_t value) {
    *(volatile uint32_t *)reg = value;
}

static uint32_t read_reg(uint32_t reg) {
    return *(volatile uint32_t *)reg;
}

// Manually setup NTSC 320x240 16bpp
void vi_init_manual(void) {
    write_reg(VI_STATUS_REG,  0x0000320E); // 16BPP, Dither, Enable
    write_reg(VI_WIDTH_REG,   320);
    write_reg(VI_INTR_REG,    0x200);
    write_reg(VI_CURRENT_REG, 0);
    write_reg(VI_BURST_REG,   0x03E52239);
    write_reg(VI_V_SYNC_REG,  0x0000020D);
    write_reg(VI_H_SYNC_REG,  0x00000C15);
    write_reg(VI_LEAP_REG,    0x0C150C15);
    write_reg(VI_H_START_REG, 0x006C02EC);
    write_reg(VI_V_START_REG, 0x002501FF);
    write_reg(VI_V_BURST_REG, 0x000E0204);
    write_reg(VI_X_SCALE_REG, 0x00000200);
    write_reg(VI_Y_SCALE_REG, 0x00000400);
    
    // Point VI to our manual buffer (Physical Address)
    // KSEG0 (Cached) -> Physical: & 0x1FFFFFFF
    uint32_t phys_addr = (uint32_t)screen_buffer & 0x1FFFFFFF;
    write_reg(VI_ORIGIN_REG, phys_addr);
}

// Simple VSync wait using VI_CURRENT_REG
void vi_wait_vsync(void) {
    while (read_reg(VI_CURRENT_REG) < 10); // Wait for top of screen
    while (read_reg(VI_CURRENT_REG) > 10); // Wait for it to start counting
}

static void draw_menu(int seq_count, const test_sequence_t *sequences) {
    console_clear();
    printf("N64 Audio Test (BARE METAL)\n");
    printf("===========================\n\n");
    for (int i = 0; i < seq_count; i++) {
        if (i == menu_selection) printf("> %d. %s\n", i + 1, sequences[i].name);
        else printf("  %d. %s\n", i + 1, sequences[i].name);
    }
    printf("\n\nControls: D-Pad to select, A to run");
}

static void draw_running(int sequence_id) {
    console_clear();
    const int seq_count_dummy; 
    // Just minimal info to avoid complex calls
    printf("Running Test %d...\n", sequence_id + 1);
    printf("Press B to return\n\n");
    
    uint32_t ai_status  = *(volatile uint32_t*)AI_STATUS_REG;
    uint32_t ai_control = *(volatile uint32_t*)AI_CONTROL_REG;
    
    printf("AI STATUS:  %08lX\n", ai_status);
    printf("AI CONTROL: %08lX\n", ai_control);
}

int main(void) {
    // 1. Initialize bare essentials
    vi_init_manual();
    joypad_init();
    timer_init(); // Keep timer for audio delays if needed
    
    // 2. Setup Console to draw to our manual buffer
    // We construct a surface_t manually to trick the console
    surface_t my_surface;
    my_surface.buffer = (void*)screen_buffer;
    my_surface.width = 320;
    my_surface.height = 240;
    my_surface.stride = 320 * 2; // 2 bytes per pixel
    my_surface.format = TYPE_RGBA16; // Standard 16-bit
    
    console_init();
    console_set_render_mode(RENDER_MANUAL);

    int seq_count;
    const test_sequence_t *sequences = get_test_sequences(&seq_count);
    
    // Clear screen initially (Black)
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

        // 3. Render directly to our buffer
        // Note: console_render writes to the provided surface pointer
        console_render(&my_surface);

        // 4. Important: Flush the Data Cache so the VI (which uses DMA) sees our changes
        data_cache_hit_writeback(screen_buffer, sizeof(screen_buffer));

        // 5. Wait for VSync to prevent flickering
        vi_wait_vsync();
    }
}