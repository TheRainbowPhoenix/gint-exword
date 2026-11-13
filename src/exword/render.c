#include <gint/display.h>
#include <gint/drivers/r61523.h>
#include <gint/image.h>
#include <stdint.h>

static uint16_t row_buffer[DWIDTH];

struct dwindow dwindow = { 0, 0, DWIDTH, DHEIGHT };

struct dwindow dwindow_set(struct dwindow window)
{
    struct dwindow previous = dwindow;
    if(window.left < 0) window.left = 0;
    if(window.top < 0) window.top = 0;
    if(window.right > DWIDTH) window.right = DWIDTH;
    if(window.bottom > DHEIGHT) window.bottom = DHEIGHT;
    if(window.right < window.left) window.right = window.left;
    if(window.bottom < window.top) window.bottom = window.top;
    dwindow = window;
    return previous;
}

void dupdate(void) { }

void dclear(color_t color)
{
    for(int x = 0; x < DWIDTH; x++) row_buffer[x] = color;
    for(int y = 0; y < DHEIGHT; y++)
        r61523_display_row(row_buffer, y);
}

void dpixel(int x, int y, int color)
{
    if(color < 0 || x < dwindow.left || x >= dwindow.right ||
       y < dwindow.top || y >= dwindow.bottom)
        return;
    r61523_set_pixel(x, y, color);
}

void drect(int x1, int y1, int x2, int y2, int color)
{
    if(color < 0) return;
    if(x1 > x2) { int t = x1; x1 = x2; x2 = t; }
    if(y1 > y2) { int t = y1; y1 = y2; y2 = t; }
    if(x1 < dwindow.left) x1 = dwindow.left;
    if(y1 < dwindow.top) y1 = dwindow.top;
    if(x2 >= dwindow.right) x2 = dwindow.right - 1;
    if(y2 >= dwindow.bottom) y2 = dwindow.bottom - 1;
    if(x1 > x2 || y1 > y2) return;
    r61523_fill_rect(x1, y1, x2, y2, color);
}

void dline(int x1, int y1, int x2, int y2, int color)
{
    if(color < 0) return;
    if(y1 == y2) { drect(x1, y1, x2, y2, color); return; }
    if(x1 == x2) { drect(x1, y1, x2, y2, color); return; }

    int dx = x2 - x1, dy = y2 - y1;
    int sx = dx >= 0 ? 1 : -1;
    int sy = dy >= 0 ? 1 : -1;
    if(dx < 0) dx = -dx;
    if(dy < 0) dy = -dy;
    int err = dx - dy;
    for(;;) {
        dpixel(x1, y1, color);
        if(x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if(e2 > -dy) { err -= dy; x1 += sx; }
        if(e2 < dx) { err += dx; y1 += sy; }
    }
}

void dcircle(int x, int y, int radius, int fill, int outline)
{
    if(radius < 0) return;
    int dx = radius, dy = 0, err = 1 - radius;
    while(dx >= dy) {
        if(fill >= 0) {
            drect(x - dx, y + dy, x + dx, y + dy, fill);
            drect(x - dx, y - dy, x + dx, y - dy, fill);
            drect(x - dy, y + dx, x + dy, y + dx, fill);
            drect(x - dy, y - dx, x + dy, y - dx, fill);
        }
        if(outline >= 0) {
            dpixel(x + dx, y + dy, outline);
            dpixel(x + dy, y + dx, outline);
            dpixel(x - dy, y + dx, outline);
            dpixel(x - dx, y + dy, outline);
            dpixel(x - dx, y - dy, outline);
            dpixel(x - dy, y - dx, outline);
            dpixel(x + dy, y - dx, outline);
            dpixel(x + dx, y - dy, outline);
        }
        dy++;
        if(err < 0) err += 2 * dy + 1;
        else { dx--; err += 2 * (dy - dx) + 1; }
    }
}

void dpoly(int const *x, int const *y, int n, int fill, int border)
{
    if(!x || !y || n < 2) return;
    if(fill >= 0 && n >= 3 && n <= 16) {
        int ymin = y[0], ymax = y[0];
        for(int i = 1; i < n; i++) {
            if(y[i] < ymin) ymin = y[i];
            if(y[i] > ymax) ymax = y[i];
        }
        if(ymin < dwindow.top) ymin = dwindow.top;
        if(ymax >= dwindow.bottom) ymax = dwindow.bottom - 1;
        for(int py = ymin; py <= ymax; py++) {
            int cuts[16], count = 0;
            for(int i = 0, j = n-1; i < n; j = i++) {
                if((y[i] <= py && y[j] > py) ||
                   (y[j] <= py && y[i] > py))
                    cuts[count++] = x[i] + (py-y[i])*(x[j]-x[i])/(y[j]-y[i]);
            }
            for(int i = 1; i < count; i++) {
                int v = cuts[i], j = i;
                while(j > 0 && cuts[j-1] > v) {
                    cuts[j] = cuts[j-1]; j--;
                }
                cuts[j] = v;
            }
            for(int i = 0; i + 1 < count; i += 2)
                drect(cuts[i], py, cuts[i+1], py, fill);
        }
    }
    if(border >= 0)
        for(int i = 0; i < n; i++)
            dline(x[i], y[i], x[(i+1)%n], y[(i+1)%n], border);
}

static uint8_t const glyphs[37][7] = {
    {14,17,17,31,17,17,17}, {30,17,17,30,17,17,30},
    {14,17,16,16,16,17,14}, {30,17,17,17,17,17,30},
    {31,16,16,30,16,16,31}, {31,16,16,30,16,16,16},
    {14,17,16,23,17,17,14}, {17,17,17,31,17,17,17},
    {14,4,4,4,4,4,14}, {7,2,2,2,18,18,12},
    {17,18,20,24,20,18,17}, {16,16,16,16,16,16,31},
    {17,27,21,21,17,17,17}, {17,25,21,19,17,17,17},
    {14,17,17,17,17,17,14}, {30,17,17,30,16,16,16},
    {14,17,17,17,21,18,13}, {30,17,17,30,20,18,17},
    {15,16,16,14,1,1,30}, {31,4,4,4,4,4,4},
    {17,17,17,17,17,17,14}, {17,17,17,17,17,10,4},
    {17,17,17,21,21,21,10}, {17,17,10,4,10,17,17},
    {17,17,10,4,4,4,4}, {31,1,2,4,8,16,31},
    {14,17,19,21,25,17,14}, {4,12,4,4,4,4,14},
    {14,17,1,2,4,8,31}, {30,1,1,14,1,1,30},
    {2,6,10,18,31,2,2}, {31,16,30,1,1,17,14},
    {6,8,16,30,17,17,14}, {31,1,2,4,8,8,8},
    {14,17,17,14,17,17,14}, {14,17,17,15,1,2,12},
    {0,0,0,0,0,0,0},
};

static uint8_t const *glyph(char ch)
{
    if(ch >= 'a' && ch <= 'z') ch -= 'a' - 'A';
    if(ch >= 'A' && ch <= 'Z') return glyphs[ch - 'A'];
    if(ch >= '0' && ch <= '9') return glyphs[ch - '0' + 26];
    return glyphs[36];
}

void dtext(int x, int y, int fg, char const *string)
{
    if(!string) return;
    for(; *string; string++, x += 6) {
        if(*string == ' ') continue;
        uint8_t const *bits = glyph(*string);
        if(*string == ':' || *string == '!') {
            for(int py = 0; py < 7; py++)
                if((*string == ':' && (py == 2 || py == 5)) ||
                   (*string == '!' && py != 5))
                    drect(x + 2, y + py, x + 2, y + py, fg);
            continue;
        }
        for(int py = 0; py < 7; py++) {
            uint8_t row = bits[py];
            for(int px = 0; px < 5; px++)
                if(row & (1 << (4-px)))
                    dpixel(x + px, y + py, fg);
        }
    }
}

void dimage(int x, int y, bopti_image_t const *image)
{
    if(!image || image->format != IMAGE_RGB565 || !image->data)
        return;
    for(int py = 0; py < image->height; py++) {
        uint16_t const *pixels = (void const *)
            ((uint8_t const *)image->data + py * image->stride);
        for(int px = 0; px < image->width; px++)
            dpixel(x+px, y+py, pixels[px]);
    }
}
