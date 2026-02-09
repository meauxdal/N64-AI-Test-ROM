#include <stdio.h>
#include <libdragon.h>
#include "audio_tests.h"

static int menu_selection = 0;
static int running_test = 0;
static int current_test_index = 0;

static void draw_menu(surface_t *disp) {
    int seq_count;
    const test_sequence_t *sequences = get_test_sequences(&seq_count);
    
    graphics_fill_screen(disp, 0);
    
    // Header
    graphics_set_color(graphics_make_color(255, 255, 255, 255), 0);
    graphics_draw_text(disp, 20, 20, "N64 Audio Interface Test ROM");
    graphics_draw_text(disp, 20, 35, "================================");
    
    for (int i = 0; i < seq_count; i++) {
        // Determine and apply the color
        uint32_t text_color;
        if (i == menu_selection) {
            text_color = graphics_make_color(255, 255, 0, 255); // Yellow
        } else {
            text_color = graphics_make_color(204, 204, 204, 255); // Grey
        }
        
        graphics_set_color(text_color, 0);
        
        char line[64];
        snprintf(line, sizeof(line), "%c %d. %s", 
                 (i == menu_selection) ? '>' : ' ',
                 i + 1, 
                 sequences[i].name);
        graphics_draw_text(disp, 30, 60 + i * 30, line);
        
        // Description - slightly darker
        graphics_set_color(graphics_make_color(136, 136, 136, 255), 0);
        graphics_draw_text(disp, 50, 75 + i * 30, sequences[i].description);
    }
    
    // Footer
    graphics_set_color(graphics_make_color(255, 255, 255, 255), 0);
    graphics_draw_text(disp, 20, 210, "D-Pad: Select  A: Run");
}

static void draw_running(surface_t *disp, int sequence_id, int test_index) {
    int seq_count;
    const test_sequence_t *sequences = get_test_sequences(&seq_count);
    const test_sequence_t *seq = &sequences[sequence_id];
    
    graphics_fill_screen(disp, 0);
    graphics_set_color(graphics_make_color(255, 255, 255, 255), 0);
    
    // Header
    char title[64];
    snprintf(title, sizeof(title), "%s", seq->name);
    graphics_draw_text(disp, 20, 20, title);
    
    // Progress indicator
    graphics_set_color(graphics_make_color(200, 200, 200, 255), 0);
    char progress[64];
    snprintf(progress, sizeof(progress), "Test %d of %d", test_index + 1, seq->test_count);
    graphics_draw_text(disp, 20, 40, progress);
    
    // Draw progress bar
    int bar_x = 20;
    int bar_y = 55;
    int bar_width = 280;
    int bar_height = 8;
    
    // Background bar (grey)
    graphics_set_color(graphics_make_color(60, 60, 60, 255), 0);
    graphics_draw_box(disp, bar_x, bar_y, bar_width, bar_height, graphics_make_color(60, 60, 60, 255));
    
    // Progress bar (yellow)
    int progress_width = (bar_width * (test_index + 1)) / seq->test_count;
    graphics_draw_box(disp, bar_x, bar_y, progress_width, bar_height, graphics_make_color(255, 255, 0, 255));
    
    if (test_index < seq->test_count) {
        test_config_t *test = &seq->tests[test_index];
        
        // Current test parameters - larger, more readable
        graphics_set_color(graphics_make_color(255, 255, 255, 255), 0);
        graphics_draw_text(disp, 20, 80, "CURRENT TONE:");
        
        char freq[64];
        snprintf(freq, sizeof(freq), "Frequency:  %lu Hz", (unsigned long)test->frequency);
        graphics_draw_text(disp, 30, 100, freq);
        
        char amp[64];
        snprintf(amp, sizeof(amp), "Amplitude:  0x%04X", test->amplitude);
        graphics_draw_text(disp, 30, 120, amp);
        
        char samples[64];
        snprintf(samples, sizeof(samples), "Samples:    %u", test->sample_count);
        graphics_draw_text(disp, 30, 140, samples);
        
        char duration[64];
        snprintf(duration, sizeof(duration), "Wait Time:  %lu ms", (unsigned long)test->wait_ms);
        graphics_draw_text(disp, 30, 160, duration);
        
        // Visual indicator for amplitude
        graphics_set_color(graphics_make_color(100, 200, 100, 255), 0);
        int amp_bar_width = (test->amplitude * 200) / 0x7FFF;
        graphics_draw_box(disp, 30, 175, amp_bar_width, 6, graphics_make_color(100, 200, 100, 255));
    } else {
        // Test complete
        graphics_set_color(graphics_make_color(100, 255, 100, 255), 0);
        graphics_draw_text(disp, 20, 100, "SEQUENCE COMPLETE!");
    }
    
    // Footer
    graphics_set_color(graphics_make_color(180, 180, 180, 255), 0);
    graphics_draw_text(disp, 20, 210, "Test auto-returns to menu");
}

int main(void) {
    // RESOLUTION_320x240 @ 16BPP
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 2, GAMMA_NONE, ANTIALIAS_RESAMPLE_FETCH_ALWAYS);
    joypad_init();
    timer_init();
    
    while (1) {
        surface_t *disp = display_get();
        
        int seq_count;
        const test_sequence_t *sequences = get_test_sequences(&seq_count);
        
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
                current_test_index = 0;  // Start from first test
            }
            
            display_show(disp);
        } else {
            const test_sequence_t *seq = &sequences[menu_selection];
            
            if (current_test_index < seq->test_count) {
                // Draw current test info
                draw_running(disp, menu_selection, current_test_index);
                display_show(disp);
                
                // Run this single test
                run_single_test(menu_selection, current_test_index);
                
                // Move to next test
                current_test_index++;
            } else {
                // All tests complete - show final screen briefly
                draw_running(disp, menu_selection, current_test_index);
                display_show(disp);
                wait_ms(2000);
                
                // Return to menu
                running_test = 0;
                current_test_index = 0;
            }
        }
    }
}
