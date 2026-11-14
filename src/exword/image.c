#include <gint/image.h>

/* Loop symbols select operations in rendering commands. */
#define LOOP(name) __attribute__((no_icf)) void name(void) {}
LOOP(gint_image_rgb16_normal)
LOOP(gint_image_rgb16_clearbg)
LOOP(gint_image_rgb16_swapcolor)
LOOP(gint_image_rgb16_dye)
LOOP(gint_image_p8_normal)
LOOP(gint_image_p8_clearbg)
LOOP(gint_image_p8_swapcolor)
LOOP(gint_image_p8_dye)
LOOP(gint_image_p4_normal)
LOOP(gint_image_p4_clearbg)
LOOP(gint_image_p4_clearbg_alt)
LOOP(gint_image_p4_swapcolor)
LOOP(gint_image_p4_dye)
#undef LOOP

static void *render(int width, struct gint_image_cmd *cmd, int format)
{
    void const *loop = cmd->loop;
    int clear = loop == gint_image_rgb16_clearbg || loop == gint_image_p8_clearbg ||
        loop == gint_image_p4_clearbg || loop == gint_image_p4_clearbg_alt;
    int swap = loop == gint_image_rgb16_swapcolor || loop == gint_image_p8_swapcolor ||
        loop == gint_image_p4_swapcolor;
    int dye = loop == gint_image_rgb16_dye || loop == gint_image_p8_dye ||
        loop == gint_image_p4_dye;
    int stride = cmd->input_stride;
    if(cmd->effect & 1) stride = -stride;
    uint8_t const *input = cmd->input;
    uint16_t *output = (void *)cmd->output;
    for(int y = 0; y < cmd->lines; y++) {
        for(int x = 0; x < cmd->columns; x++) {
            int pixel;
            if(format == 16) {
                pixel = (input[2*x] << 8) | input[2*x+1];
            }
            else if(format == 8) {
                pixel = (int8_t)input[x];
            }
            else {
                int offset = x + cmd->edge_1;
                pixel = input[offset >> 1];
                pixel = (offset & 1) ? pixel & 15 : pixel >> 4;
            }
            int match = (clear || swap || dye) && (uint16_t)pixel == cmd->color_1;
            if((clear || dye) && match) continue;
            int color = format == 16 ? pixel : cmd->palette[pixel];
            if(swap && match) color = cmd->color_2;
            if(dye) color = cmd->color_2;
            int dx = (cmd->effect & 2) ? cmd->columns-1-x : x;
            output[dx] = color;
        }
        input += stride;
        output += width;
    }
    return (void *)input;
}

void *gint_image_rgb16_loop(int width, struct gint_image_cmd *cmd)
{
    return render(width, cmd, 16);
}

void *gint_image_p8_loop(int width, struct gint_image_cmd *cmd)
{
    return render(width, cmd, 8);
}

void *gint_image_p4_loop(int width, struct gint_image_cmd *cmd)
{
    return render(width, cmd, 4);
}
