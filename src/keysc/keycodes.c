#include <gint/config.h>
#include <gint/keyboard.h>

/* keycode_function(): Identify keys F1 .. F6 */
int keycode_function(int keycode)
{
	if((keycode & 0xf0) != 0x90) return -1;
	return keycode & 0x0f;
}

/* keycode_digit(): Identify keys 0 .. 9 */
int keycode_digit(int keycode)
{
#if GINT_HW_EX
	if(keycode >= KEY_EX_Q && keycode <= KEY_EX_P)
		return (keycode - KEY_EX_Q + 1) % 10;
	return -1;
#else
	int row = keycode >> 4;
	int col = keycode & 0xf;

	if(col > 3) return -1;
	if(keycode == 0x11) return 0;

	int digit = 3 * row + col - 6;
	if(digit >= 1 && digit <= 9) return digit;

	return -1;
#endif
}
