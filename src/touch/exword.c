#include <gint/config.h>

#if GINT_HW_EX

#include <gint/drivers/touch.h>
#include <gint/display.h>
#include <gint/keyboard.h>
#include <stdint.h>

#define IIC_DATA  (*(volatile uint8_t *)0xa4470000)
#define IIC_CTRL  (*(volatile uint8_t *)0xa4470004)
#define IIC_STAT  (*(volatile uint8_t *)0xa4470008)
#define IIC_INT   (*(volatile uint8_t *)0xa447000c)
#define IIC_LOW   (*(volatile uint8_t *)0xa4470010)
#define IIC_HIGH  (*(volatile uint8_t *)0xa4470014)
#define PEN_PORT  (*(volatile uint8_t *)0xa4050134)
#define FRQCR     (*(volatile uint32_t *)0xa4150000)

#define IIC_LIMIT 100000

/* ROM32A LCD_ValidateSubDisplayWindow() fallback calibration. */
#define EX_X_OFFSET 0x7e
#define EX_Y_OFFSET 0xe9
#define EX_X_SCALE  0x689
#define EX_Y_SCALE  0xb17
#define EX_X_TARGET 0x1fd
#define EX_Y_TARGET 0x12f
#define EX_X_MARGIN ((DWIDTH - EX_X_TARGET) / 2)
#define EX_Y_MARGIN ((DHEIGHT - EX_Y_TARGET) / 2)

static struct exword_touch_debug debug_scan;

static struct {
    int xmin, xmax, ymin, ymax;
    int xflip, yflip;
    int last_x, last_y;
    int down;
} touch_state = {
    .xmin = EX_X_OFFSET,
    .xmax = EX_X_OFFSET + EX_X_TARGET * EX_X_SCALE / 256,
    .ymin = EX_Y_OFFSET,
    .ymax = EX_Y_OFFSET + EX_Y_TARGET * EX_Y_SCALE / 256,
};

void touch_exword_get_last_scan(struct exword_touch_debug *out)
{
    if(out) *out = debug_scan;
}

void touch_exword_calibrate(int xmin, int xmax, int ymin, int ymax,
    int xflip, int yflip)
{
    if(xmin >= xmax || ymin >= ymax) return;
    touch_state.xmin = xmin;
    touch_state.xmax = xmax;
    touch_state.ymin = ymin;
    touch_state.ymax = ymax;
    touch_state.xflip = xflip;
    touch_state.yflip = yflip;
}

static int wait_flag(uint8_t flags)
{
    for(int timeout = IIC_LIMIT; timeout > 0; timeout--)
        if(IIC_STAT & flags) return 1;
    return 0;
}

static void i2c_begin(void)
{
    IIC_CTRL |= 0x80;
    if((FRQCR & 15) == 3) {
        IIC_LOW = 0x12;
        IIC_HIGH = 0x0c;
    }
    else {
        IIC_LOW = 0x25;
        IIC_HIGH = 0x19;
    }
    IIC_STAT &= 0xf1;
}

static int i2c_start(void)
{
    IIC_INT = 3;
    IIC_CTRL = 0x94;
    if(!wait_flag(1)) return 0;
    IIC_INT = 2;
    return 1;
}

static int i2c_send(uint8_t byte, uint8_t control)
{
    IIC_DATA = byte;
    if(control) IIC_CTRL = control;
    if(control == 0x81) {
        IIC_STAT &= ~2;
        if(!wait_flag(0x0a)) return 0;
    }
    IIC_STAT &= ~2;
    return wait_flag(0x0a);
}

static void i2c_end(void)
{
    IIC_INT = 0;
    IIC_LOW = 0;
    IIC_HIGH = 0;
    IIC_CTRL &= 0x7f;
}

static int touch_command(uint8_t command)
{
    i2c_begin();
    if(!i2c_start() || !i2c_send(0x90, 0) ||
       !i2c_send(command, 0x90)) {
        i2c_end();
        return 0;
    }
    IIC_STAT &= ~2;
    int ok = wait_flag(0x10);
    i2c_end();
    return ok;
}

static int touch_read_word(uint16_t *out)
{
    uint8_t high, low;
    i2c_begin();
    if(!i2c_start() || !i2c_send(0x91, 0x81)) goto failed;
    IIC_STAT &= ~2;
    if(!wait_flag(1)) goto failed;
    IIC_INT = 2;
    high = IIC_DATA;
    if(!wait_flag(0x0a)) goto failed;
    IIC_CTRL = 0xc0;
    IIC_STAT &= ~2;
    if(!wait_flag(1)) goto failed;
    IIC_INT = 2;
    low = IIC_DATA;
    i2c_end();
    *out = ((uint16_t)high << 8) | low;
    return 1;
failed:
    i2c_end();
    return 0;
}

static int touch_axis(uint8_t command, int low, int high, int *value)
{
    uint16_t samples[5], discard;
    if(!touch_command(command) || !touch_read_word(&discard))
        return 0;
    for(int i = 0; i < 5; i++) {
        if(!touch_read_word(&samples[i])) return 0;
        samples[i] >>= 4;
    }
    int imin = 0, imax = 0;
    for(int i = 1; i < 5; i++) {
        if(samples[i] < samples[imin]) imin = i;
        if(samples[i] > samples[imax]) imax = i;
    }
    int avg = -1;
    for(int i = 0; i < 5; i++) {
        if(i == imin || i == imax) continue;
        if(samples[i] < low || samples[i] > high) return 0;
        avg = (avg < 0) ? samples[i] : (avg + samples[i]) / 2;
    }
    *value = avg;
    return avg >= 0;
}

static int normalize_raw(int n, int min, int max, int extent, int margin,
    int flip)
{
    int p = (n - min) * extent / (max - min);
    return margin + (flip ? extent - p : p);
}

static int clamp_screen(int p, int size)
{
    if(p < 0) return 0;
    if(p >= size) return size - 1;
    return p;
}

key_event_t touch_scan(void)
{
    key_event_t ev = { .type = KEYEV_NONE };
    int raw_x = 0, raw_y = 0;
    int down = 0;
    debug_scan.pen_down = ((PEN_PORT & 1) == 0);
    debug_scan.valid = 0;

    if(debug_scan.pen_down) {
        int x_ok = touch_axis(0xc4, 0x50, 0xff5, &raw_x);
        int y_ok = touch_axis(0xd4, 0x70, 0xff1, &raw_y);
        (void)touch_command(0xc0);
        down = x_ok && y_ok && ((PEN_PORT & 1) == 0);
    }

    if(down) {
        int x = normalize_raw(raw_x, touch_state.xmin, touch_state.xmax,
            EX_X_TARGET, EX_X_MARGIN, touch_state.xflip);
        int y = normalize_raw(raw_y, touch_state.ymin, touch_state.ymax,
            EX_Y_TARGET, EX_Y_MARGIN, touch_state.yflip);

        debug_scan.raw_x = raw_x;
        debug_scan.raw_y = raw_y;
        debug_scan.mapped_x = x;
        debug_scan.mapped_y = y;
        debug_scan.screen_x = clamp_screen(x, DWIDTH);
        debug_scan.screen_y = clamp_screen(y, DHEIGHT);
        debug_scan.valid = 1;

        x = debug_scan.screen_x;
        y = debug_scan.screen_y;
        if(!touch_state.down)
            ev.type = KEYEV_TOUCH_DOWN;
        else if(x != touch_state.last_x || y != touch_state.last_y)
            ev.type = KEYEV_TOUCH_DRAG;
        touch_state.last_x = x;
        touch_state.last_y = y;
    }
    else if(touch_state.down) {
        ev.type = KEYEV_TOUCH_UP;
    }

    if(ev.type != KEYEV_NONE) {
        ev.x = touch_state.last_x;
        ev.y = touch_state.last_y;
    }
    touch_state.down = down;
    return ev;
}

#endif
