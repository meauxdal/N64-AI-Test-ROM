#include <libdragon.h>
#include <string.h>
#include <stdbool.h>

/* ---------- Editor Geometry ---------- */

#define COLS 64
#define ROWS 26
#define DOC_ROWS 256

static char doc[DOC_ROWS][COLS];

static int cx = 0;         /* cursor x */
static int cy = 0;         /* cursor y in document */
static int scroll = 0;     /* top visible line */


/* ---------- Character Maps ---------- */

static char shifted_number_map[10] =
{
    ')','!','@','#','$','%','^','&','*','('
};


/* ---------- Editor Helpers ---------- */

static void doc_clear(void)
{
    for (int y = 0; y < DOC_ROWS; y++)
        for (int x = 0; x < COLS; x++)
            doc[y][x] = ' ';
}

static void scroll_adjust(void)
{
    if (cy < scroll)
        scroll = cy;

    if (cy >= scroll + ROWS)
        scroll = cy - ROWS + 1;
}

static void insert_char(char c)
{
    for (int x = COLS - 1; x > cx; x--)
        doc[cy][x] = doc[cy][x - 1];

    doc[cy][cx] = c;

    if (cx < COLS - 1)
        cx++;
}

static void delete_char(void)
{
    for (int x = cx; x < COLS - 1; x++)
        doc[cy][x] = doc[cy][x + 1];

    doc[cy][COLS - 1] = ' ';
}

static void newline(void)
{
    if (cy < DOC_ROWS - 1) {
        cy++;
        cx = 0;
    }
}


/* ---------- Rendering ---------- */

static void render(void)
{
    console_clear();

    for (int y = 0; y < ROWS; y++)
    {
        int line = y + scroll;

        console_set_cursor(0, y);

        for (int x = 0; x < COLS; x++)
            console_putc(doc[line][x]);
    }

    /* Status bar */
    console_set_cursor(0, ROWS);
    printf("N64 Type  |  Ln %d Col %d  | Ctrl+L Clear",
           cy + 1, cx + 1);

    console_set_cursor(cx, cy - scroll);
}


/* ---------- Keyboard Handling ---------- */

static bool key_pressed(uint8_t key, usb_keyboard_state_t *kbd)
{
    for (int i = 0; i < 6; i++)
        if (kbd->keys[i] == key)
            return true;

    return false;
}

static bool shift_down(usb_keyboard_state_t *kbd)
{
    return (kbd->modifier &
        (KEY_MOD_LSHIFT | KEY_MOD_RSHIFT));
}

static bool ctrl_down(usb_keyboard_state_t *kbd)
{
    return (kbd->modifier &
        (KEY_MOD_LCTRL | KEY_MOD_RCTRL));
}

static void handle_keyboard(usb_keyboard_state_t *kbd)
{
    bool shift = shift_down(kbd);
    bool ctrl  = ctrl_down(kbd);

    /* Ctrl shortcuts */
    if (ctrl && key_pressed(HID_KEY_L, kbd)) {
        doc_clear();
        cx = cy = scroll = 0;
        return;
    }

    /* Cursor movement */
    if (key_pressed(HID_KEY_LEFT, kbd) && cx > 0) cx--;
    if (key_pressed(HID_KEY_RIGHT, kbd) && cx < COLS-1) cx++;
    if (key_pressed(HID_KEY_UP, kbd) && cy > 0) cy--;
    if (key_pressed(HID_KEY_DOWN, kbd) && cy < DOC_ROWS-1) cy++;

    /* Editing */
    if (key_pressed(HID_KEY_BACKSPACE, kbd) && cx > 0) {
        cx--;
        delete_char();
    }

    if (key_pressed(HID_KEY_DELETE, kbd))
        delete_char();

    if (key_pressed(HID_KEY_ENTER, kbd))
        newline();

    /* Character input */
    for (int i = 0; i < 6; i++)
    {
        uint8_t k = kbd->keys[i];
        char out = 0;

        if (k >= HID_KEY_A && k <= HID_KEY_Z) {
            out = (shift) ? 'A' + (k - HID_KEY_A)
                          : 'a' + (k - HID_KEY_A);
        }

        else if (k >= HID_KEY_1 && k <= HID_KEY_9) {
            out = shift ? shifted_number_map[k - HID_KEY_1 + 1]
                        : '1' + (k - HID_KEY_1);
        }

        else if (k == HID_KEY_0) {
            out = shift ? shifted_number_map[0] : '0';
        }

        else if (k == HID_KEY_SPACE) out = ' ';

        if (out)
            insert_char(out);
    }

    scroll_adjust();
}


/* ---------- Entry ---------- */

int main(void)
{
    display_init(RESOLUTION_640x480, DEPTH_16_BPP, 2,
                 GAMMA_NONE, FILTERS_RESAMPLE);

    console_init();
    usb_init();
    joypad_init();

    doc_clear();

    while (1)
    {
        usb_poll();
        joypad_poll();

        usb_keyboard_state_t *kbd =
            usb_keyboard_get_state();

        if (kbd)
            handle_keyboard(kbd);

        render();
        display_show(console_get_render_buffer());
    }

    return 0;
}
