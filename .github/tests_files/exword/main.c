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

static char const *key_name(int code)
{
    static struct { int code; char const *name; } const names[] = {
        {KEY_F1,"F1"}, {KEY_F2,"F2"}, {KEY_F3,"F3"},
        {KEY_F4,"F4"}, {KEY_F5,"F5"}, {KEY_F6,"F6"},
        {KEY_F7,"F7"}, {KEY_F8,"F8"},
        {KEY_SHIFT,"SHIFT"}, {KEY_ALPHA,"ALPHA"}, {KEY_DEL,"DEL"},
        {KEY_EXIT,"EXIT"}, {KEY_EXE,"EXE"}, {KEY_ACON,"ACON"},
        {KEY_UP,"UP"}, {KEY_DOWN,"DOWN"}, {KEY_LEFT,"LEFT"},
        {KEY_RIGHT,"RIGHT"}, {KEY_PAGEUP,"PAGEUP"}, {KEY_PAGEDOWN,"PAGEDOWN"},
        {KEY_EX_Q,"EX Q"}, {KEY_EX_W,"EX W"}, {KEY_EX_E,"EX E"},
        {KEY_EX_R,"EX R"}, {KEY_EX_T,"EX T"}, {KEY_EX_Y,"EX Y"},
        {KEY_EX_U,"EX U"}, {KEY_EX_I,"EX I"}, {KEY_EX_O,"EX O"},
        {KEY_EX_P,"EX P"}, {KEY_EX_A,"EX A"}, {KEY_EX_S,"EX S"},
        {KEY_EX_D,"EX D"}, {KEY_EX_F,"EX F"}, {KEY_EX_G,"EX G"},
        {KEY_EX_H,"EX H"}, {KEY_EX_J,"EX J"}, {KEY_EX_K,"EX K"},
        {KEY_EX_L,"EX L"}, {KEY_EX_Z,"EX Z"}, {KEY_EX_X,"EX X"},
        {KEY_EX_C,"EX C"}, {KEY_EX_V,"EX V"}, {KEY_EX_B,"EX B"},
        {KEY_EX_N,"EX N"}, {KEY_EX_M,"EX M"},
        {KEY_EX_HISTORY,"EX HISTORY"}, {KEY_EX_ZOOM,"EX ZOOM"},
        {KEY_EX_JUMP,"EX JUMP"}, {KEY_EX_AUDIO,"EX AUDIO"},
        {KEY_EX_LAYOUT,"EX LAYOUT"},
    };
    if(code == 0) return "UNMAPPED";
    for(unsigned i = 0; i < sizeof(names)/sizeof(names[0]); i++)
        if(names[i].code == code) return names[i].name;
    return "UNKNOWN";
}

#define KEY_LOG_SIZE 18
static key_event_t key_log[KEY_LOG_SIZE];
static int key_log_next, key_log_count;

static void key_test_screen(void)
{
    key_log_next = key_log_count = 0;
    dclear(C_WHITE);
    drect(0, 0, 110, 23, C_LIGHT);
    dtext(8, 8, C_BLACK, "DRAW");
    dtext(140, 8, C_BLACK, "GINTT KEY TEST");
    drect(400, 0, DWIDTH-1, 23, C_LIGHT);
    dtext(420, 8, C_BLACK, "QUIT");
    dtext(8, 32, C_BLACK, "PRESS EACH KEY  EXIT AND ACON ARE LOGGED TOO");
    dtext(8, 48, C_BLACK, "EVENT");
    dtext(74, 48, C_BLACK, "CODE");
    dtext(128, 48, C_BLACK, "HEX");
    dtext(170, 48, C_BLACK, "NAME");
    dtext(398, 48, C_BLACK, "ROW");
    dtext(458, 48, C_BLACK, "COL");
    dupdate();
}

static void key_test_row(int slot, int background)
{
    key_event_t ev = key_log[slot];
    int y = 66 + slot * 14;
    char digits[8];
    char hex[] = "0X00";
    char const *hex_digits = "0123456789ABCDEF";
    hex[2] = hex_digits[ev.key >> 4];
    hex[3] = hex_digits[ev.key & 15];
    drect(0, y-2, DWIDTH-1, y+10, background);
    dtext(8, y, C_BLACK, ev.type == KEYEV_DOWN ? "DOWN" :
        ev.type == KEYEV_UP ? "UP" : "HOLD");
    number(digits, ev.key);
    dtext(74, y, C_BLACK, digits);
    dtext(128, y, C_BLACK, hex);
    dtext(170, y, C_BLACK, key_name(ev.key));
    number(digits, ev.row);
    dtext(398, y, C_BLACK, digits);
    number(digits, ev.col);
    dtext(458, y, C_BLACK, digits);
    dupdate_rect(0, y-2, DWIDTH-1, y+10);
}

static void key_test_event(key_event_t ev)
{
    if(key_log_count)
        key_test_row((key_log_next + KEY_LOG_SIZE-1) % KEY_LOG_SIZE, C_WHITE);
    key_log[key_log_next] = ev;
    key_test_row(key_log_next, C_LIGHT);
    key_log_next = (key_log_next + 1) % KEY_LOG_SIZE;
    if(key_log_count < KEY_LOG_SIZE) key_log_count++;
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

static int show_debug(int force)
{
    static struct exword_touch_debug previous;
    struct exword_touch_debug info;
    touch_exword_get_last_scan(&info);
    if(!force && info.raw_x == previous.raw_x && info.raw_y == previous.raw_y &&
       info.mapped_x == previous.mapped_x && info.mapped_y == previous.mapped_y &&
       info.pen_down == previous.pen_down && info.valid == previous.valid &&
       info.accepted_scans == previous.accepted_scans &&
       info.rejected_scans == previous.rejected_scans) return 0;
    debug_line(18, "RAW X:", info.raw_x, info.raw_y);
    debug_line(30, "MAP X:", info.mapped_x, info.mapped_y);
    debug_line(42, "PEN VALID:", info.pen_down, info.valid);
    debug_line(54, "OK / BAD:", info.accepted_scans, info.rejected_scans);
    previous = info;
    return 1;
}

static void clear_screen(void)
{
    dclear(C_WHITE);
    drect(0, 0, DWIDTH-1, 54, C_LIGHT);
    dtext(8, 5, C_BLACK, "TOUCH DRAW  EXE CLEAR  EXIT QUIT");
    struct exword_touch_calibration calib;
    touch_exword_get_calibration(&calib);
    dtext(400, 5, C_BLACK, calib.source == EXWORD_TOUCH_CALIB_OS ? "CAL OS" :
        calib.source == EXWORD_TOUCH_CALIB_CUSTOM ? "CAL CUSTOM" : "CAL DEFAULT");
    show_debug(1);

    drect(12, 65, 65, 87, C_RED);
    dline(85, 65, 150, 87, C_GREEN);
    dcircle(180, 76, 9, C_BLUE, C_BLACK);
    int x[3] = { 216, 233, 248 };
    int y[3] = { 87, 65, 87 };
    dpoly(x, y, 3, C_GREEN, C_BLACK);
    dimage(270, 69, &image);
    dpixel(300, 76, C_BLACK);
    drect(310, 65, 420, 87, C_LIGHT);
    dtext(325, 72, C_BLACK, "KEY TEST");

    dupdate();
}

int main(void)
{
    clear_screen();
    int old_x = -1, old_y = -1;
    unsigned idle_polls = 0;
    int keys_mode = 0;

    for(;;) {
        key_event_t ev = pollevent();
        if(keys_mode) {
            if(ev.type == KEYEV_DOWN || ev.type == KEYEV_UP || ev.type == KEYEV_HOLD)
                key_test_event(ev);
            else if(ev.type == KEYEV_TOUCH_DOWN && ev.y < 24) {
                if(ev.x >= 400) {
                    touch_exword_quit();
                    return -2;
                }
                if(ev.x <= 110) {
                    keys_mode = 0;
                    clear_screen();
                    old_x = old_y = -1;
                }
            }
            continue;
        }
        if(ev.type == KEYEV_TOUCH_DOWN && ev.x >= 310 && ev.x <= 420 &&
           ev.y >= 65 && ev.y <= 87) {
            keys_mode = 1;
            key_test_screen();
            old_x = old_y = -1;
            continue;
        }
        if(ev.type != KEYEV_NONE) {
            if(ev.type == KEYEV_DOWN) {
                if(ev.key == KEY_EXIT || ev.key == KEY_ACON) {
                    touch_exword_quit();
                    return -2;
                }
                if(ev.key == KEY_EXE) {
                    clear_screen();
                    old_x = old_y = -1;
                }
            }
            else if(ev.type == KEYEV_TOUCH_DOWN) {
                old_x = ev.x;
                old_y = ev.y;
                if(ev.y >= 105) {
                    dcircle(ev.x, ev.y, 2, C_BLUE, C_BLUE);
                    dupdate_rect(ev.x-2, ev.y-2, ev.x+2, ev.y+2);
                }
            }
            else if(ev.type == KEYEV_TOUCH_DRAG) {
                if(old_x >= 0 && old_y >= 105 && ev.y >= 105) {
                    dline(old_x, old_y, ev.x, ev.y, C_BLACK);
                    int x1 = old_x < ev.x ? old_x : ev.x;
                    int x2 = old_x > ev.x ? old_x : ev.x;
                    int y1 = old_y < ev.y ? old_y : ev.y;
                    int y2 = old_y > ev.y ? old_y : ev.y;
                    dupdate_rect(x1, y1, x2, y2);
                }
                old_x = ev.x;
                old_y = ev.y;
            }
            else if(ev.type == KEYEV_TOUCH_UP) {
                old_x = old_y = -1;
            }
        }
        /* refreshing every point stalls input. refresh every 16 polls, even on continuous stroke. */
        if(++idle_polls >= 16) {
            idle_polls = 0;
            if(show_debug(0)) dupdate_rect(0, 18, DWIDTH-1, 64);
        }
    }
}
