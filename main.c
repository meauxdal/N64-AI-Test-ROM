#include <stdio.h>
#include <libdragon.h>
#include "audio_tests.h"

// Define register constants for cleaner code
#define AI_BASE          0xA4500000
#define AI_CONTROL_REG   (AI_BASE + 0x08)
#define AI_STATUS_REG    (AI_BASE + 0x0C)
#define AI_DACRATE_REG   (AI_BASE + 0x10)

static int menu_selection = 0;
static int running_test = 0;

static void draw_menu(int seq_count, const test_sequence_t *sequences) {
    console_clear();
    
    printf("N64 Audio Interface Test ROM (v3.0)\n");
    printf("===================================\n\n");
    
    for (int i = 0; i < seq_count; i++) {
        if (i == menu_selection) {
            printf("> %d. %s\n", i + 1, sequences[i].name);
            printf("   %s\n\n", sequences[i].description);
        } else {
            printf("  %d. %s\n\n", i + 1, sequences[i].name);
        }
    }
    
    printf("\n\n\nControls: D-Pad to select, A to run");
}

static void draw_running(int sequence_id) {
    int seq_count;
    const test_sequence_t *sequences = get_test_sequences(&seq_count);
    
    console_clear();
    printf("Running: %s\n", sequences[sequence_id].name);
    printf("===================================\n\n");
    printf("Test in progress...\n");
    printf("Audio output active\n\n");
    printf("Press B to return to menu\n");
    
    // Read hardware registers using KSEG1 (Uncached) pointers
    // This ensures we see the REAL hardware state, not a CPU cache
    uint32_t ai_status  = *(volatile uint32_t*)AI_STATUS_REG;
    uint32_t ai_control = *(volatile uint32_t*)AI_CONTROL_REG;
    uint32_t ai_dacrate = *(volatile uint32_t*)AI_DACRATE_REG;

    printf("\n--- AI HW REGISTERS ---\n");
    printf("STATUS:  0x%08lX\n", ai_status);
    printf("CONTROL: 0x%08lX\n", ai_control);
    printf("DACRATE: 0x%08lX\n", ai_dacrate);

    // Decoding for user clarity
    if (ai_status & 0x80000000) printf("[ FIFO FULL ] ");
    if (ai_status & 0x40000000) printf("[ DMA BUSY ] ");
    printf("\n");
}

int main(void) {    
    // INIT SYSTEM
    // 3 Buffers (Triple Buffering) is the key to preventing RSP timeouts.
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 3, GAMMA_NONE, ANTIALIAS_RESAMPLE_FETCH_ALWAYS);
    
    console_init();
    console_set_render_mode(RENDER_MANUAL);
    
    // Ensure interrupts are on so the VI (Video Interface) can flip pages
    enable_interrupts();
    
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
                // Run the test logic ONCE. 
                // This prevents restarting the DMA 60 times a second.
                run_test_sequence(menu_selection); 
            }
            draw_menu(seq_count, sequences);
        } else {
            if (keys.b) {
                running_test = 0;
                // Optional: Stop audio here if you have a stop function
            }
            draw_running(menu_selection);
        }
        
        // SAFE RENDERING PIPELINE
        // 1. Check if a buffer is available.
        surface_t *disp = display_get(); 
        
        // 2. Render content
        graphics_fill_screen(disp, 0);
        console_render();
        
        // 3. Sync the RDP (Graphics Processor) to ensure drawing is done
        rdp_sync_pipe();
        
        // 4. Show the frame
        display_show(disp);
        
        // 5. No manual wait needed with 3 buffers, but a tiny sleep 
        // helps the emulator thread yielding.
        // wait_ms(1) is enough.
    }
}