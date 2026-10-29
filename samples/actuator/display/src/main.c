/************************************************************************
 * @file main.c
 *
 * @brief ZBook Display sample: shows the ZBook logo, then cycles through shapes and text.
 * @author João Matheus Nascimento Dias (joao.dias@edge.ufal.br)
 * @version 0.1
 * @date 27/10/2026
 *
 * @copyright Centro de Inovação EDGE 2026. Todos os direitos reservados.
 *
 ***********************************************************************/

#include "actuator/zbook_display.h"

#include <zephyr/kernel.h>

#define FRAME_TIME_MS 3000

static void draw_circles_frame(void)
{
	zbook_display_clear();

	zbook_display_set_draw_mode(ZBOOK_DISPLAY_PIXEL_WHITE);
	for (uint8_t radius = 28; radius <= 30; radius++) {
		zbook_display_draw_circle(ZBOOK_DISPLAY_VISIBLE_W / 2, ZBOOK_DISPLAY_VISIBLE_H / 2,
					  radius);
	}

	zbook_display_set_draw_mode(ZBOOK_DISPLAY_PIXEL_INVERT);
	zbook_display_print((const uint8_t *)"Hello, World!", 27, 11, FONT_SIZE_10, 1);
	zbook_display_set_draw_mode(ZBOOK_DISPLAY_PIXEL_WHITE);

	zbook_display_flush();
}

static void draw_shapes_frame(void)
{
	zbook_display_clear();

	zbook_display_draw_rect(0, 0, ZBOOK_DISPLAY_VISIBLE_H - 1, ZBOOK_DISPLAY_VISIBLE_W - 1,
				false);
	zbook_display_draw_rect(8, 8, 20, 30, true);
	zbook_display_draw_line(48, 8, 118, 28);
	zbook_display_draw_line(48, 28, 118, 8);

	zbook_display_print((const uint8_t *)"ZBook Interface", 36, 8, FONT_SIZE_7, 1);
	zbook_display_print((const uint8_t *)"Display", 46, 8, FONT_SIZE_7, 2);

	zbook_display_flush();
}

int main(void)
{
	zbook_display_init();

	while (true) {
		draw_circles_frame();
		k_msleep(FRAME_TIME_MS);

		draw_shapes_frame();
		k_msleep(FRAME_TIME_MS);
	}

	return 0;
}
