#include <gint/display.h>
#include <gint/image.h>
#include <gint/keyboard.h>
#include <gint/keycodes.h>
#include <gint/drivers/touch.h>

static uint16_t const image_pixels[8 * 8] = {
    C_RED, C_RED, C_RED, C_RED, C_RED, C_RED, C_RED, C_RED,
    C_RED, C_WHITE, C_WHITE, C_WHITE, C_WHITE, C_WHITE, C_WHITE, C_RED,
    C_RED, C_WHITE, C_BLUE, C_BLUE, C_BLUE, C_BLUE, C_WHITE, C_RED,
    C_RED, C_WHITE, C_BLUE, C_GREEN, C_GREEN, C_BLUE, C_WHITE, C_RED,
    C_RED, C_WHITE, C_BLUE, C_GREEN, C_GREEN, C_BLUE, C_WHITE, C_RED,
    C_RED, C_WHITE, C_BLUE, C_BLUE, C_BLUE, C_BLUE, C_WHITE, C_RED,
    C_RED, C_WHITE, C_WHITE, C_WHITE, C_WHITE, C_WHITE, C_RED,
    C_RED, C_RED, C_RED, C_RED, C_RED, C_RED, C_RED, C_RED,
};

static bopti_image_t const image = {
    .format = IMAGE_RGB565,
    .width = 8,
    .height = 8,
    .stride = 16,
    .data = (void *)image_pixels,
};

static void number(char *out, int value)
{
    if(value < 0) {
        *out++ = '-';
        value = -value;
    }
    int divisor = 10000;
    while(divisor) {
        int digit = value / divisor;
        *out++ = '0' + digit % 10;
        value %= divisor;
        divisor /= 10;
    }
    *out = 0;
}

static void debug_line(int y, char const *title, int x, int z)
{
    char digits[8];
    drect(0, y, DWIDTH-1, y+10, C_LIGHT);
    dtext(8, y+2, C_BLACK, title);
    number(digits, x);
    dtext(96, y+2, C_BLACK, digits);
    dtext(180, y+2, C_BLACK, "Y:");
    number(digits, z);
    dtext(210, y+2, C_BLACK, digits);
}

static void show_debug(void)
{
    struct exword_touch_debug info;
    touch_exword_get_last_scan(&info);
    debug_line(18, "RAW X:", info.raw_x, info.raw_y);
    debug_line(30, "MAP X:", info.mapped_x, info.mapped_y);
    debug_line(42, "PEN VALID:", info.pen_down, info.valid);
}

static void clear_screen(void)
{
    dclear(C_WHITE);
    drect(0, 0, DWIDTH-1, 54, C_LIGHT);
    dtext(8, 5, C_BLACK, "TOUCH DRAW  EXE CLEAR  EXIT QUIT");
    show_debug();

    drect(12, 65, 65, 87, C_RED);
    dline(85, 65, 150, 87, C_GREEN);
    dcircle(180, 76, 9, C_BLUE, C_BLACK);
    int x[3] = { 216, 233, 248 };
    int y[3] = { 87, 65, 87 };
    dpoly(x, y, 3, C_GREEN, C_BLACK);
    dimage(270, 69, &image);
    dpixel(300, 76, C_BLACK);
    dtext(325, 72, C_BLACK, "GINTT");

    dupdate();
}

int main(void)
{
    clear_screen();
    int old_x = -1, old_y = -1;
    unsigned idle_polls = 0;

    for(;;) {
        key_event_t ev = pollevent();
        while(ev.type != KEYEV_NONE) {
            if(ev.type == KEYEV_DOWN) {
                if(ev.key == KEY_EXIT || ev.key == KEY_ACON)
                    return -2;
                if(ev.key == KEY_EXE) {
                    clear_screen();
                    old_x = old_y = -1;
                }
            }
            else if(ev.type == KEYEV_TOUCH_DOWN) {
                old_x = ev.x;
                old_y = ev.y;
                if(ev.y >= 105)
                    dcircle(ev.x, ev.y, 2, C_BLUE, C_BLUE);
                show_debug();
            }
            else if(ev.type == KEYEV_TOUCH_DRAG) {
                if(old_x >= 0 && old_y >= 105 && ev.y >= 105)
                    dline(old_x, old_y, ev.x, ev.y, C_BLACK);
                old_x = ev.x;
                old_y = ev.y;
                show_debug();
            }
            else if(ev.type == KEYEV_TOUCH_UP) {
                old_x = old_y = -1;
                show_debug();
            }
            ev = pollevent();
        }
        if(++idle_polls >= 4096) {
            idle_polls = 0;
            show_debug();
        }
        dupdate();
    }
}
