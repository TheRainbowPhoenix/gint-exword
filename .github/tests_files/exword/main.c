#include <gint/drivers/r61523.h>
#include <gint/keyboard.h>
#include <gint/keycodes.h>
#include <gint/display.h>

static uint16_t line[DWIDTH];

static void draw_frame(void)
{
    for(int y = 0; y < DHEIGHT; y++) {
        for(int x = 0; x < DWIDTH; x++) {
            int half = (y - 64) >> 1;
            int inside = y >= 64 && y <= 256 &&
                x >= 264 - half && x <= 264 + half;
            line[x] = inside ? C_BLUE : C_WHITE;
        }
        r61523_display_row(line, y);
    }
}

static void draw_key_indicator(int key)
{
    uint16_t color = key ? C_GREEN : C_WHITE;
    if(key == KEY_EXIT || key == KEY_ACON)
        color = C_RED;
    for(int i = 0; i < 10; i++) {
        for(int x = 0; x < DWIDTH; x++)
            line[x] = color;
        r61523_display_row(line, i);
    }
}

int main(void)
{
    draw_frame();
    draw_key_indicator(0);

    for(;;) {
        key_event_t ev = pollevent();
        if(ev.type == KEYEV_DOWN) {
            draw_key_indicator(ev.key);
            if(ev.key == KEY_EXIT || ev.key == KEY_ACON)
                return -2;
        }
        else if(ev.type == KEYEV_UP)
            draw_key_indicator(0);
    }
}
