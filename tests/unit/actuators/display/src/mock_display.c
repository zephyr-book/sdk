/************************************************************************
 * @file mock_display.c
 *
 * @brief Mock of the deps/display driver, records every call it gets.
 * @author João Matheus Nascimento Dias (joao.dias@edge.ufal.br)
 * @version 0.1
 * @date 27/10/2026
 *
 * @copyright Centro de Inovação EDGE 2026. Todos os direitos reservados.
 *
 ***********************************************************************/

#include "mock_display.h"

#include <string.h>

struct mock_display mock_display;

void mock_display_reset(void)
{
	memset(&mock_display, 0, sizeof(mock_display));
}

void display_init(bool is_dcdc_dependent)
{
	mock_display.init.calls++;
	mock_display.init.order = ++mock_display.seq;
	mock_display.init.is_dcdc_dependent = is_dcdc_dependent;
}

void display_set_contrast(uint8_t amount)
{
	mock_display.contrast.calls++;
	mock_display.contrast.order = ++mock_display.seq;
	mock_display.contrast.amount = amount;
}

void display_disable(void)
{
	mock_display.disable_calls++;
}

void display_enable(void)
{
	mock_display.enable_calls++;
}

void display_clear(void)
{
	mock_display.clear_calls++;
}

void display_flush(void)
{
	mock_display.flush_calls++;
}

void display_draw_line(uint8_t origin_x, uint8_t origin_y, uint8_t end_x, uint8_t end_y)
{
	mock_display.line.calls++;
	mock_display.line.origin_x = origin_x;
	mock_display.line.origin_y = origin_y;
	mock_display.line.end_x = end_x;
	mock_display.line.end_y = end_y;
}

void display_draw_rect(uint8_t top, uint8_t left, uint8_t height, uint8_t width, bool filled)
{
	mock_display.rect.calls++;
	mock_display.rect.top = top;
	mock_display.rect.left = left;
	mock_display.rect.height = height;
	mock_display.rect.width = width;
	mock_display.rect.filled = filled;
}

void display_draw_circle(uint8_t center_x, uint8_t center_y, uint8_t radius)
{
	mock_display.circle.calls++;
	mock_display.circle.center_x = center_x;
	mock_display.circle.center_y = center_y;
	mock_display.circle.radius = radius;
}

void display_draw_bitmap(const uint8_t *bmp, uint8_t top, uint8_t left, uint8_t width,
			 uint8_t height, uint8_t scale)
{
	mock_display.bitmap.calls++;
	mock_display.bitmap.bmp = bmp;
	mock_display.bitmap.top = top;
	mock_display.bitmap.left = left;
	mock_display.bitmap.width = width;
	mock_display.bitmap.height = height;
	mock_display.bitmap.scale = scale;
}

uint16_t display_print(const uint8_t *str, uint8_t top, uint8_t left, enum font_sizes font,
		       uint8_t scale)
{
	mock_display.print.calls++;
	mock_display.print.str = str;
	mock_display.print.top = top;
	mock_display.print.left = left;
	mock_display.print.font = font;
	mock_display.print.scale = scale;

	return MOCK_DISPLAY_PRINT_RET;
}

void display_set_draw_mode(enum display_draw_mode mode)
{
	mock_display.draw_mode.calls++;
	mock_display.draw_mode.mode = mode;
}
