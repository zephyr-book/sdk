/************************************************************************
 * @file main.c
 *
 * @brief ZBook Display fonts sample: shows the ZBook logo, then cycles through the font
 * sizes and scales.
 * @author João Matheus Nascimento Dias (joao.dias@edge.ufal.br)
 * @version 0.1
 * @date 28/10/2026
 *
 * @copyright Centro de Inovação EDGE 2026. Todos os direitos reservados.
 *
 ***********************************************************************/

#include "actuator/zbook_display.h"

#include <zephyr/kernel.h>

#define FRAME_TIME_MS 3000
#define MARGIN        2
#define FONT_7_H      7
#define FONT_10_H     10

static void draw_fonts_frame(void)
{
	zbook_display_clear();

	zbook_display_print((const uint8_t *)"Fonte 5x7", 0, MARGIN, FONT_SIZE_7, 1);
	zbook_display_print((const uint8_t *)"ABCDEFGHIJKLMNOPQRST", 9, MARGIN, FONT_SIZE_7, 1);
	zbook_display_print((const uint8_t *)"abcdefg 0123456789", 18, MARGIN, FONT_SIZE_7, 1);

	zbook_display_print((const uint8_t *)"Fonte 7x10", 30, MARGIN, FONT_SIZE_10, 1);
	zbook_display_print((const uint8_t *)"ABCDEFGHIJKLMNO", 42, MARGIN, FONT_SIZE_10, 1);
	zbook_display_print((const uint8_t *)"abcdefg 0123456", 54, MARGIN, FONT_SIZE_10, 1);

	zbook_display_flush();
}

static void draw_scales_frame(enum font_sizes font, uint8_t height)
{
	uint8_t top = 0;

	zbook_display_clear();

	zbook_display_print((const uint8_t *)"x1 Ab", top, MARGIN, font, 1);
	top += height + 2;
	zbook_display_print((const uint8_t *)"x2 Ab", top, MARGIN, font, 2);
	top += (height * 2) + 2;
	zbook_display_print((const uint8_t *)"x3 Ab", top, MARGIN, font, 3);

	zbook_display_flush();
}

static void draw_mixed_frame(void)
{
	const uint8_t baseline = 40;
	uint8_t left = MARGIN;

	zbook_display_clear();

	zbook_display_print((const uint8_t *)"Fontes juntas", 2, MARGIN, FONT_SIZE_7, 1);

	left += zbook_display_print((const uint8_t *)"5x7", baseline - FONT_7_H, left, FONT_SIZE_7,
				    1);
	left += zbook_display_print((const uint8_t *)"7x10", baseline - FONT_10_H, left,
				    FONT_SIZE_10, 1);
	left += zbook_display_print((const uint8_t *)"x2", baseline - (FONT_10_H * 2), left,
				    FONT_SIZE_10, 2);

	zbook_display_draw_line(MARGIN, baseline + 2, left, baseline + 2);

	zbook_display_flush();
}

int main(void)
{
	zbook_display_init();

	while (true) {
		draw_fonts_frame();
		k_msleep(FRAME_TIME_MS);

		draw_scales_frame(FONT_SIZE_7, FONT_7_H);
		k_msleep(FRAME_TIME_MS);

		draw_scales_frame(FONT_SIZE_10, FONT_10_H);
		k_msleep(FRAME_TIME_MS);

		draw_mixed_frame();
		k_msleep(FRAME_TIME_MS);
	}

	return 0;
}
