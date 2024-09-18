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
	int row = keycode >> 4;
	int col = keycode & 0xf;

	if(col > 3) return -1;
	if(keycode == 0x11) return 0;

	int digit = 3 * row + col - 6;
	if(digit >= 1 && digit <= 9) return digit;

	return -1;
}

/* keycode_alpha(): Identify ASCII characters */
uint8_t keycode_alpha(int keycode)
{
	const int Row = keycode >> 4;
	const int Column = keycode & 0xf;

	switch (Row)
	{
		case 6: return 'A' + Column - 1;
		case 5: return 'G' + Column - 1;
		case 4:
			switch (Column)
			{
				case 1:
				case 2:
				case 3: return 'M' + Column - 1;
				default: -1;
			}
		case 3: return 'P' + Column - 1;
		case 2: return 'U' + Column - 1;
		case 1:
			switch (Column)
			{
				case 1: return 'Z';
				case 2: return ' ';
				case 3: return '\"';
				default: return -1;
			}
		default: return -1;
	}
}
