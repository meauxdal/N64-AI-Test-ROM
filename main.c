#include <stdio.h>
#include <libdragon.h>
#include "audio_tests.h"

static int menu_selection = 0;
static int running_test = 0;

static void draw_menu(surface_t *disp) {
    int seq_count;
    const test_sequence_t *sequences = get_test_sequences(&seq_count);
    
    graphics_fill_screen(disp, 0);
    
    // Header
    graphics_set_color(graphics_make_color(255, 255, 255, 255), 0);
    graphics_draw_text(disp, 40, 30, "N64 Audio Interface Test ROM");
    graphics_draw_text(disp, 40, 50, "================================");
    
    for (int i = 0; i < seq_count; i++) {
        // FIX: Determine and actually USE the color
        uint32_t text_color;
        if (i == menu_selection) {
            text_color = graphics_make_color(255, 255, 0, 255); // Yellow
        } else {
            text_color = graphics_make_color(204, 204, 204, 255); // Grey
        }
        
        // Apply the calculated selection color
        graphics_set_color(text_color, 0);
        
        char line[64];
        snprintf(line, sizeof(line), "%c %d. %s", 
                 (i == menu_selection) ? '>' : ' ',
                 i + 1, 
                 sequences[i].name);
        graphics_draw_text(disp, 60, 80 + i * 30, line);
        
        // Description - slightly darker
        graphics_set_color(graphics_make_color(136, 136, 136, 255), 0);
        graphics_draw_text(disp, 80, 95 + i * 30, sequences[i].description);
    }
    
    // Footer
    graphics_set_color(graphics_make_color(255, 255, 255, 255), 0);
    graphics_draw_text(disp, 40, 220, "Controls: D-Pad to select, A to run");
}

static void draw_running(surface_t *disp, int sequence_id) {
    int seq_count;
    const test_sequence_t *sequences = get_test_sequences(&seq_count);
    
    graphics_fill_screen(disp, 0);
    // FIX: Removed 0xFFFFFFFF which caused the vertical bars
    graphics_set_color(graphics_make_color(255, 255, 255, 255), 0);
    
    char title[64];
    snprintf(title, sizeof(title), "Running: %s", sequences[sequence_id].name);
    graphics_draw_text(disp, 40, 30, title);
    
    graphics_draw_text(disp, 40, 60, "Test in progress...");
    graphics_draw_text(disp, 40, 90, "Audio output active");
    graphics_draw_text(disp, 40, 200, "Test will auto-return to menu when complete");
}

int main(void) {
    // RESOLUTION_320x240 @ 16BPP
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 2, GAMMA_NONE, ANTIALIAS_RESAMPLE_FETCH_ALWAYS);
    joypad_init();
    timer_init();
    
    while (1) {
        surface_t *disp = display_get();
        
        int seq_count;
        get_test_sequences(&seq_count);
        
        if (!running_test) {
            draw_menu(disp);
            
            joypad_poll();
            joypad_buttons_t keys = joypad_get_buttons_pressed(JOYPAD_PORT_1);
            
            if (keys.d_up && menu_selection > 0) {
                menu_selection--;
            }
            if (keys.d_down && menu_selection < seq_count - 1) {
                menu_selection++;
            }
            if (keys.a) {
                running_test = 1;
            }
            
            display_show(disp);
        } else {
            draw_running(disp, menu_selection);
            
            // Show the "Running test" screen BEFORE starting audio
            display_show(disp);
            
            // Now run the audio test - user can see the screen while audio plays
            run_test_sequence(menu_selection);
            
            // FIX: Automatically return to menu after test completes
            running_test = 0;
        }
    }
}
