#include <stdio.h>
#include <libdragon.h>
#include "audio_tests.h"

static int menu_selection = 0;
static int running_test = 0;

static void draw_menu(surface_t *disp) {
    int seq_count;
    const test_sequence_t *sequences = get_test_sequences(&seq_count);
    
    graphics_fill_screen(disp, 0x000000FF);
    
    graphics_set_color(0xFFFFFFFF, 0x00000000);
    graphics_draw_text(disp, 40, 30, "N64 Audio Interface Test ROM");
    graphics_draw_text(disp, 40, 50, "================================");
    
    for (int i = 0; i < seq_count; i++) {
        uint32_t color = (i == menu_selection) ? 0xFFFF00FF : 0xCCCCCCFF;
        graphics_set_color(color, 0x00000000);
        
        char line[64];
        snprintf(line, sizeof(line), "%c %d. %s", 
                 (i == menu_selection) ? '>' : ' ',
                 i + 1, 
                 sequences[i].name);
        graphics_draw_text(disp, 60, 80 + i * 30, line);
        
        graphics_set_color(0x888888FF, 0x00000000);
        graphics_draw_text(disp, 80, 95 + i * 30, sequences[i].description);
    }
    
    graphics_set_color(0xFFFFFFFF, 0x00000000);
    graphics_draw_text(disp, 40, 220, "Controls: D-Pad to select, A to run");
}

static void draw_running(surface_t *disp, int sequence_id) {
    int seq_count;
    const test_sequence_t *sequences = get_test_sequences(&seq_count);
    
    graphics_fill_screen(disp, 0x000000FF);
    graphics_set_color(0xFFFFFFFF, 0x00000000);
    
    char title[64];
    snprintf(title, sizeof(title), "Running: %s", sequences[sequence_id].name);
    graphics_draw_text(disp, 40, 30, title);
    
    graphics_draw_text(disp, 40, 60, "Test in progress...");
    graphics_draw_text(disp, 40, 90, "Audio output active");
    graphics_draw_text(disp, 40, 200, "Press B to return to menu");
}

int main(void) {
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 2, GAMMA_NONE, FILTERS_RESAMPLE);
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
        } else {
            draw_running(disp, menu_selection);
            
            run_test_sequence(menu_selection);
            
            joypad_poll();
            joypad_buttons_t keys = joypad_get_buttons_pressed(JOYPAD_PORT_1);
            if (keys.b) {
                running_test = 0;
            }
        }
        
        display_show(disp);
    }
}
