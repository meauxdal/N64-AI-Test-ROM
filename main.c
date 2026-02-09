#include <libdragon.h>
#include <string.h>

#define SCREEN_COLS  64
#define SCREEN_ROWS  28
#define TEXT_SIZE    (SCREEN_COLS * SCREEN_ROWS)

static char text_buffer[TEXT_SIZE];
static int cursor_x = 0;
static int cursor_y = 0;

/* --- Helper --- */
static void put_char(char c)
{
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
        return;
    }

    if (cursor_x >= SCREEN_COLS) {
        cursor_x = 0;
        cursor_y++;
    }

    if (cursor_y >= SCREEN_ROWS)
        cursor_y = SCREEN_ROWS - 1;

    text_buffer[cursor_y * SCREEN_COLS + cursor_x] = c;
    cursor_x++;
}

/* --- Backspace support --- */
static void backspace(void)
{
    if (cursor_x > 0) {
        cursor_x--;
    } else if (cursor_y > 0) {
        cursor_y--;
        cursor_x = SCREEN_COLS - 1;
    }

    text_buffer[cursor_y * SCREEN_COLS + cursor_x] = ' ';
}

/* --- Draw entire buffer --- */
static void draw_text(void)
{
    console_clear();

    for (int y = 0; y < SCREEN_ROWS; y++) {
        console_set_cursor(0, y);

        for (int x = 0; x < SCREEN_COLS; x++) {
            char c = text_buffer[y * SCREEN_COLS + x];
            if (!c) c = ' ';
            console_putc(c);
        }
    }

    console_set_cursor(cursor_x, cursor_y);
}

/* --- Translate keyboard input --- */
static void handle_key(uint8_t keycode)
{
    /* Extremely small mapping for MVP.
       Expand later if needed. */

    if (keycode >= HID_KEY_A && keycode <= HID_KEY_Z) {
        char c = 'a' + (keycode - HID_KEY_A);
        put_char(c);
    }
    else if (keycode >= HID_KEY_1 && keycode <= HID_KEY_9) {
        char c = '1' + (keycode - HID_KEY_1);
        put_char(c);
    }
    else if (keycode == HID_KEY_0) {
        put_char('0');
    }
    else if (keycode == HID_KEY_SPACE) {
        put_char(' ');
    }
    else if (keycode == HID_KEY_ENTER) {
        put_char('\n');
    }
    else if (keycode == HID_KEY_BACKSPACE) {
        backspace();
    }
}

/* --- Entry --- */
int main(void)
{
    display_init(RESOLUTION_640x480, DEPTH_16_BPP, 2, GAMMA_NONE, FILTERS_RESAMPLE);
    console_init();

    usb_init();
    joypad_init();

    memset(text_buffer, ' ', sizeof(text_buffer));

    while (1)
    {
        joypad_poll();

        usb_poll();

        /* Check keyboard events */
        usb_keyboard_state_t *kbd = usb_keyboard_get_state();

        if (kbd)
        {
            for (int i = 0; i < 6; i++) {
                uint8_t key = kbd->keys[i];
                if (key)
                    handle_key(key);
            }
        }

        draw_text();

        display_show(console_get_render_buffer());
    }

    return 0;
}
