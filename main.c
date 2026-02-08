#include <stdio.h>
#include <libdragon.h>
#include "audio_tests.h"

#define VI_BASE          0xA4400000
#define VI_STATUS_REG    (VI_BASE + 0x00)
#define AI_BASE          0xA4500000
#define AI_CONTROL_REG   (AI_BASE + 0x08)
#define AI_STATUS_REG    (AI_BASE + 0x0C)
#define AI_DACRATE_REG   (AI_BASE + 0x10)

static int menu_selection = 0;
static int running_test = 0;

static void draw_menu(int seq_count, const test_sequence_t *sequences) {
    console_clear();
    
    printf("N64 Audio Interface Test ROM\n");
    printf("================================\n\n");
    
    for (int i = 0; i < seq_count; i++) {
        if (i == menu_selection) {
            printf("> %d. %s\n", i + 1, sequences[i].name);
            printf("   %s\n\n", sequences[i].description);
        } else {
            printf("  %d. %s\n\n", i + 1, sequences[i].name);
        }
    }
    
    printf("\n\n\n\nControls: D-Pad to select, A to run");
}

static void draw_running(int sequence_id) {
    int seq_count;
    const test_sequence_t *sequences = get_test_sequences(&seq_count);
    
    console_clear();
    printf("Running: %s\n", sequences[sequence_id].name);
    printf("================================\n\n");
    printf("Test in progress...\n");
    printf("Audio output active\n\n");
    printf("Wait for completion or\n");
    printf("Press B to return to menu\n");
	
	uint32_t ai_status  = *(volatile uint32_t*)AI_STATUS_REG;
    uint32_t ai_control = *(volatile uint32_t*)AI_CONTROL_REG;
    uint32_t ai_dacrate = *(volatile uint32_t*)AI_DACRATE_REG;

    printf("\n--- AI HW REGISTERS ---\n");
    printf("STATUS:  0x%08X\n", (unsigned int)ai_status);
    printf("CONTROL: 0x%08lX\n", ai_control);
    printf("DACRATE: 0x%08lX\n", ai_dacrate);
}

int main(void) {    
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 2, GAMMA_NONE, ANTIALIAS_RESAMPLE_FETCH_ALWAYS);
    
    console_init();
    console_set_render_mode(RENDER_MANUAL);
    
    joypad_init();
    timer_init();
    
    int seq_count;
    const test_sequence_t *sequences = get_test_sequences(&seq_count);
    
    while (1) {
        joypad_poll();
        joypad_buttons_t keys = joypad_get_buttons_pressed(JOYPAD_PORT_1);
        
        if (!running_test) {
            if (keys.d_up && menu_selection > 0) menu_selection--;
            if (keys.d_down && menu_selection < seq_count - 1) menu_selection++;
            if (keys.a) {
                running_test = 1;
                run_test_sequence(menu_selection); // Start audio ONCE
            }
            draw_menu(seq_count, sequences);
        } else {
            if (keys.b) {
                running_test = 0;
                // Optional: add a function here to stop audio
            }
            draw_running(menu_selection); // Just update the UI and Registers
            // REMOVED: run_test_sequence(menu_selection);
        }
        
        surface_t *disp = display_try_get();
        
        if (disp) {
            graphics_fill_screen(disp, 0);
            console_render();
            display_show(disp);
        }

        wait_ms(32); 
    }
}