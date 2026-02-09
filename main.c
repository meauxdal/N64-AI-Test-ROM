static void draw_running(surface_t *disp, int sequence_id, int test_index) {
    int seq_count;
    const test_sequence_t *sequences = get_test_sequences(&seq_count);
    const test_sequence_t *seq = &sequences[sequence_id];
    
    graphics_fill_screen(disp, 0);
    graphics_set_color(graphics_make_color(255, 255, 255, 255), 0);
    
    // Margins adjusted to 25px from 20px for safety
    graphics_draw_text(disp, 25, 20, seq->name);
    
    if (test_index < seq->test_count) {
        graphics_set_color(graphics_make_color(200, 200, 200, 255), 0);
        char progress[32];
        snprintf(progress, sizeof(progress), "Step %d/%d", test_index + 1, seq->test_count);
        graphics_draw_text(disp, 25, 40, progress);
        
        // Shortened labels to prevent overflow
        test_config_t *test = &seq->tests[test_index];
        char f_txt[32], a_txt[32], s_txt[32], w_txt[32];
        snprintf(f_txt, sizeof(f_txt), "Freq: %lu Hz", (unsigned long)test->frequency);
        snprintf(a_txt, sizeof(a_txt), "Amp:  0x%04X", test->amplitude);
        snprintf(s_txt, sizeof(s_txt), "Samples: %u", test->sample_count);
        snprintf(w_txt, sizeof(w_txt), "Wait: %lu ms", (unsigned long)test->wait_ms);
        
        graphics_draw_text(disp, 30, 70, f_txt);
        graphics_draw_text(disp, 30, 85, a_txt);
        graphics_draw_text(disp, 30, 100, s_txt);
        graphics_draw_text(disp, 30, 115, w_txt);
        
        // Progress Bar (narrowed to 260px)
        graphics_draw_box(disp, 30, 140, 260, 6, graphics_make_color(60, 60, 60, 255));
        int p_w = (260 * (test_index + 1)) / seq->test_count;
        graphics_draw_box(disp, 30, 140, p_w, 6, graphics_make_color(255, 255, 0, 255));
    } else {
        graphics_set_color(graphics_make_color(100, 255, 100, 255), 0);
        graphics_draw_text(disp, 25, 70, "COMPLETE!");
    }
    
    graphics_set_color(graphics_make_color(150, 150, 150, 255), 0);
    graphics_draw_text(disp, 25, 210, "B: Return to Menu");
}