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
    C_RED, C_WHITE, C_WHITE, C_WHITE, C_WHITE, C_WHITE, C_WHITE, C_RED,
    C_RED, C_RED, C_RED, C_RED, C_RED, C_RED, C_RED, C_RED,
};

static bopti_image_t const image = {
    .format = IMAGE_RGB565,
    .width = 8,
    .height = 8,
    .stride = 16,
    .data = (void *)image_pixels,
};

static void clear_screen(void)
{
    dclear(C_WHITE);
    drect(0, 0, DWIDTH-1, 19, C_LIGHT);
    dtext(8, 6, C_BLACK, "TOUCH DRAW  EXE CLEAR  EXIT QUIT");

    drect(12, 27, 65, 39, C_RED);
    dline(85, 27, 150, 39, C_GREEN);
    dcircle(180, 33, 7, C_BLUE, C_BLACK);
    int x[3] = { 216, 233, 248 };
    int y[3] = { 39, 24, 39 };
    dpoly(x, y, 3, C_GREEN, C_BLACK);
    dimage(270, 26, &image);
    dpixel(300, 33, C_BLACK);
    dtext(325, 28, C_BLACK, "GINTT");

    dupdate();
}

int main(void)
{
    clear_screen();

    int old_x = -1, old_y = -1;

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
                if(ev.y >= 44)
                    dcircle(ev.x, ev.y, 2, C_BLUE, C_BLUE);
            }
            else if(ev.type == KEYEV_TOUCH_DRAG) {
                if(old_x >= 0 && old_y >= 44 && ev.y >= 44)
                    dline(old_x, old_y, ev.x, ev.y, C_BLACK);
                old_x = ev.x;
                old_y = ev.y;
            }
            else if(ev.type == KEYEV_TOUCH_UP) {
                old_x = old_y = -1;
            }
            ev = pollevent();
        }
        dupdate();
    }
}
