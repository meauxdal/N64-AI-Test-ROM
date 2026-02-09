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
    
    // Header - Safe margin 25
    graphics_set_color(graphics_make_color(255, 255, 255, 255), 0);
    graphics_draw_text(disp, 25, 20, " N64 Audio Interface Test ROM");
    graphics_draw_text(disp, 25, 32, "-------------------------");
    
    for (int i = 0; i < seq_count; i++) {
        uint32_t text_color = (i == menu_selection) ? 
            graphics_make_color(255, 255, 0, 255) : 
            graphics_make_color(180, 180, 180, 255);
        
        graphics_set_color(text_color, 0);
        char line[48];
        snprintf(line, sizeof(line), "%s %d. %s", 
                 (i == menu_selection) ? ">" : " ", i + 1, sequences[i].name);
        graphics_draw_text(disp, 30, 60 + i * 35, line);
        
        graphics_set_color(graphics_make_color(130, 130, 130, 255), 0);
        graphics_draw_text(disp, 45, 75 + i * 35, sequences[i].description);
    }
    
    graphics_set_color(graphics_make_color(200, 200, 200, 255), 0);
    graphics_draw_text(disp, 25, 210, "DPAD: Select  A: Run Sequence");
}

static void draw_running(surface_t *disp, int sequence_id, int test_index) {
    int seq_count;
    const test_sequence_t *sequences = get_test_sequences(&seq_count);
    const test_sequence_t *seq = &sequences[sequence_id];
    
    graphics_fill_screen(disp, 0);
    graphics_set_color(graphics_make_color(255, 255, 255, 255), 0);
    
    graphics_draw_text(disp, 25, 20, seq->name);
    
    if (test_index < seq->test_count) {
        graphics_set_color(graphics_make_color(180, 180, 180, 255), 0);
        char prog[32];
        snprintf(prog, sizeof(prog), "Test: %d / %d", test_index + 1, seq->test_count);
        graphics_draw_text(disp, 25, 40, prog);
        
        test_config_t *test = &seq->tests[test_index];
        char f_txt[32], a_txt[32], s_txt[32], w_txt[32];
        snprintf(f_txt, sizeof(f_txt), "Freq: %lu Hz", (unsigned long)test->frequency);
        snprintf(a_txt, sizeof(a_txt), "Amp:  0x%04X", test->amplitude);
        snprintf(s_txt, sizeof(s_txt), "Samples: %u", test->sample_count);
        snprintf(w_txt, sizeof(w_txt), "Wait: %lu ms", (unsigned long)test->wait_ms);
        
        graphics_set_color(graphics_make_color(255, 255, 255, 255), 0);
        graphics_draw_text(disp, 35, 70, f_txt);
        graphics_draw_text(disp, 35, 85, a_txt);
        graphics_draw_text(disp, 35, 100, s_txt);
        graphics_draw_text(disp, 35, 115, w_txt);
        
        // Progress Bar
        graphics_draw_box(disp, 35, 140, 250, 6, graphics_make_color(50, 50, 50, 255));
        int p_w = (250 * (test_index + 1)) / seq->test_count;
        graphics_draw_box(disp, 35, 140, p_w, 6, graphics_make_color(0, 255, 0, 255));
    } else {
        graphics_set_color(graphics_make_color(0, 255, 0, 255), 0);
        graphics_draw_text(disp, 25, 80, "SEQUENCE COMPLETE");
    }
    
    graphics_set_color(graphics_make_color(150, 150, 150, 255), 0);
    graphics_draw_text(disp, 25, 210, "B: Return to Menu");
}

int main(void) {
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 2, GAMMA_NONE, ANTIALIAS_RESAMPLE_FETCH_ALWAYS);
    joypad_init();
    timer_init();
    
    while (1) {
        int seq_count;
        get_test_sequences(&seq_count);
        
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
            int count;
            const test_sequence_t *sequences = get_test_sequences(&count);
            const test_sequence_t *seq = &sequences[menu_selection];
            
            if (current_test_index < seq->test_count) {
                surface_t *disp = display_get();
                draw_running(disp, menu_selection, current_test_index);
                display_show(disp);
                
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
                
                wait_ms_with_abort(1500);
                running_test = 0;
                current_test_index = 0;
            }
        }
    }
}