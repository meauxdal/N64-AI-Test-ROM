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
    
    graphics_set_color(graphics_make_color(255, 255, 255, 255), 0);
    graphics_draw_text(disp, 20, 20, "N64 Audio Interface Test ROM");
    graphics_draw_text(disp, 20, 35, "================================");
    
    for (int i = 0; i < seq_count; i++) {
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
        
        graphics_set_color(graphics_make_color(136, 136, 136, 255), 0);
        graphics_draw_text(disp, 50, 75 + i * 30, sequences[i].description);
    }
    
    graphics_set_color(graphics_make_color(255, 255, 255, 255), 0);
    graphics_draw_text(disp, 20, 210, "D-Pad: Select  A: Run");
}

static void draw_running(surface_t *disp, int sequence_id, int test_index) {
    int seq_count;
    const test_sequence_t *sequences = get_test_sequences(&seq_count);
    const test_sequence_t *seq = &sequences[sequence_id];
    
    graphics_fill_screen(disp, 0);
    graphics_set_color(graphics_make_color(255, 255, 255, 255), 0);
    
    char title[64];
    snprintf(title, sizeof(title), "%s", seq->name);
    graphics_draw_text(disp, 20, 20, title);
    
    if (test_index < seq->test_count) {
        graphics_set_color(graphics_make_color(200, 200, 200, 255), 0);
        char progress[64];
        snprintf(progress, sizeof(progress), "Test %d of %d", test_index + 1, seq->test_count);
        graphics_draw_text(disp, 20, 40, progress);
        
        int bar_x = 20, bar_y = 55, bar_width = 280, bar_height = 8;
        graphics_set_color(graphics_make_color(60, 60, 60, 255), 0);
        graphics_draw_box(disp, bar_x, bar_y, bar_width, bar_height, graphics_make_color(60, 60, 60, 255));
        
        int progress_width = (bar_width * (test_index + 1)) / seq->test_count;
        graphics_draw_box(disp, bar_x, bar_y, progress_width, bar_height, graphics_make_color(255, 255, 0, 255));
        
        test_config_t *test = &seq->tests[test_index];
        graphics_set_color(graphics_make_color(255, 255, 255, 255), 0);
        graphics_draw_text(disp, 20, 80, "CURRENT TONE:");
        
        char freq[64], amp[64], samples[64], duration[64];
        snprintf(freq, sizeof(freq), "Frequency:  %lu Hz", (unsigned long)test->frequency);
        snprintf(amp, sizeof(amp), "Amplitude:  0x%04X", test->amplitude);
        snprintf(samples, sizeof(samples), "Samples:    %u", test->sample_count);
        snprintf(duration, sizeof(duration), "Wait Time:  %lu ms", (unsigned long)test->wait_ms);
        
        graphics_draw_text(disp, 30, 100, freq);
        graphics_draw_text(disp, 30, 120, amp);
        graphics_draw_text(disp, 30, 140, samples);
        graphics_draw_text(disp, 30, 160, duration);
        
        graphics_set_color(graphics_make_color(100, 200, 100, 255), 0);
        int amp_bar_width = (test->amplitude * 200) / 0x7FFF;
        graphics_draw_box(disp, 30, 175, amp_bar_width, 6, graphics_make_color(100, 200, 100, 255));
    } else {
        graphics_set_color(graphics_make_color(100, 255, 100, 255), 0);
        graphics_draw_text(disp, 20, 70, "SEQUENCE COMPLETE!");
        graphics_set_color(graphics_make_color(200, 200, 200, 255), 0);
        char complete[64];
        snprintf(complete, sizeof(complete), "Completed all %d tests", seq->test_count);
        graphics_draw_text(disp, 20, 100, complete);
    }
    
    graphics_set_color(graphics_make_color(180, 180, 180, 255), 0);
    graphics_draw_text(disp, 20, 210, "Press B to abort and return to menu");
}

int main(void) {
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 2, GAMMA_NONE, ANTIALIAS_RESAMPLE_FETCH_ALWAYS);
    joypad_init();
    timer_init();
    
    while (1) {
        int seq_count;
        const test_sequence_t *sequences = get_test_sequences(&seq_count);
        
        if (!running_test) {
            surface_t *disp = display_get();
            draw_menu(disp);
            display_show(disp);
            
            joypad_poll();
            joypad_buttons_t keys = joypad_get_buttons_pressed(JOYPAD_PORT_1);
            
            if (keys.d_up && menu_selection > 0) menu_selection--;
            if (keys.d_down && menu_selection < seq_count - 1) menu_selection++;
            if (keys.a) {
                running_test = 1;
                current_test_index = 0;
            }
        } else {
            const test_sequence_t *seq = &sequences[menu_selection];
            
            if (current_test_index < seq->test_count) {
                surface_t *disp = display_get();
                draw_running(disp, menu_selection, current_test_index);
                display_show(disp);
                
                // If run_single_test returns 1, it means B was pressed
                if (run_single_test(menu_selection, current_test_index)) {
                    running_test = 0;
                    current_test_index = 0;
                } else {
                    current_test_index++;
                }
            } else {
                surface_t *disp = display_get();
                draw_running(disp, menu_selection, current_test_index);
                display_show(disp);
                
                // Allow the final screen to be skipped with B
                wait_ms_with_abort(2000);
                
                running_test = 0;
                current_test_index = 0;
            }
        }
    }
}