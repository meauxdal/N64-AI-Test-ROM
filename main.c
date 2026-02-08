#include <stdio.h>
#include <libdragon.h>
#include "audio_tests.h"

static int menu_selection = 0;
static int running_test = 0;

static void draw_menu(int seq_count, const test_sequence_t *sequences) {
    console_clear();
    
    // Header
    printf("N64 Audio Interface Test ROM\n");
    printf("================================\n\n");
    
    for (int i = 0; i < seq_count; i++) {
        // Highlight logic
        if (i == menu_selection) {
            printf("> %d. %s\n", i + 1, sequences[i].name);
            printf("   %s\n\n", sequences[i].description);
        } else {
            printf("  %d. %s\n\n", i + 1, sequences[i].name);
        }
    }
    
    // Position footer at the bottom
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
}

int main(void) {
    // 1. Initialize hardware
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 2, GAMMA_NONE, ANTIALIAS_RESAMPLE_FETCH_ALWAYS);
    
    // 2. Setup Console (Improved Video)
    console_init();
    console_set_render_mode(RENDER_MANUAL);
    
    joypad_init();
    timer_init();
    
    int seq_count;
    const test_sequence_t *sequences = get_test_sequences(&seq_count);
    
    while (1) {
        // 3. Logic & Input
        joypad_poll();
        joypad_buttons_t keys = joypad_get_buttons_pressed(JOYPAD_PORT_1);
        
        if (!running_test) {
            if (keys.d_up && menu_selection > 0) menu_selection--;
            if (keys.d_down && menu_selection < seq_count - 1) menu_selection++;
            if (keys.a) running_test = 1;
            
            draw_menu(seq_count, sequences);
        } else {
            if (keys.b) running_test = 0;
            
            draw_running(menu_selection);
            run_test_sequence(menu_selection);
        }
        
        // 4. Render to screen
        surface_t *disp = display_get();
        graphics_fill_screen(disp, 0); 
        console_render();             
        display_show(disp);           
        
        // THE UNIVERSAL SYNC CALL
        while (!display_get()); 
    }
}