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

static struct {
    int xmin, xmax, ymin, ymax;
    int xflip, yflip;
    int last_x, last_y;
    int down;
} touch_state = {
    .xmin = 0x50, .xmax = 0xff5,
    .ymin = 0x70, .ymax = 0xff1,
};

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

static int normalize(int n, int min, int max, int size, int flip)
{
    int p = (n - min) * (size - 1) / (max - min);
    if(p < 0) p = 0;
    if(p >= size) p = size - 1;
    return flip ? size - 1 - p : p;
}

key_event_t touch_scan(void)
{
    key_event_t ev = { .type = KEYEV_NONE };
    int x, y;
    int down = 0;
    if((PEN_PORT & 1) == 0) {
        down = touch_axis(0xc4, 0x50, 0xff5, &x) &&
               touch_axis(0xd4, 0x70, 0xff1, &y);
        (void)touch_command(0xc0);
        if(PEN_PORT & 1) down = 0;
    }

    if(down) {
        x = normalize(x, touch_state.xmin, touch_state.xmax,
                      DWIDTH, touch_state.xflip);
        y = normalize(y, touch_state.ymin, touch_state.ymax,
                      DHEIGHT, touch_state.yflip);
        if(!touch_state.down) {
            ev.type = KEYEV_TOUCH_DOWN;
        }
        else if(x != touch_state.last_x || y != touch_state.last_y) {
            ev.type = KEYEV_TOUCH_DRAG;
        }
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
