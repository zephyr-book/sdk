/************************************************************************
 * @file main.c
 *
 * @brief Unit tests of the Zbook Display actuator interface.
 * @author João Matheus Nascimento Dias (joao.dias@edge.ufal.br)
 * @version 0.1
 * @date 27/10/2026
 *
 * @copyright Centro de Inovação EDGE 2026. Todos os direitos reservados.
 *
 ***********************************************************************/

#include "actuators/zbook_display.h"
#include "mock_display.h"

#include <zephyr/ztest.h>

static void before_each(void *fixture)
{
	ARG_UNUSED(fixture);

	mock_display_reset();
}

ZTEST_SUITE(zbook_display, NULL, NULL, before_each, NULL, NULL);

ZTEST(zbook_display, test_init_uses_kconfig_dcdc_and_contrast)
{
	zbook_display_init();

	zassert_equal(mock_display.init.calls, 1);
	zassert_true(mock_display.init.is_dcdc_dependent);
	zassert_equal(mock_display.contrast.calls, 1);
	zassert_equal(mock_display.contrast.amount, CONFIG_ZBOOK_INTERFACE_DISPLAY_INIT_CONTRAST);
	zassert_true(mock_display.init.order < mock_display.contrast.order,
		     "contrast must be set after the driver is initialized");
}

ZTEST(zbook_display, test_power_and_buffer_calls_are_forwarded)
{
	zbook_display_enable();
	zbook_display_disable();
	zbook_display_clear();
	zbook_display_flush();

	zassert_equal(mock_display.enable_calls, 1);
	zassert_equal(mock_display.disable_calls, 1);
	zassert_equal(mock_display.clear_calls, 1);
	zassert_equal(mock_display.flush_calls, 1);
}

ZTEST(zbook_display, test_draw_line_forwards_arguments)
{
	zbook_display_draw_line(1, 2, 3, 4);

	zassert_equal(mock_display.line.calls, 1);
	zassert_equal(mock_display.line.origin_x, 1);
	zassert_equal(mock_display.line.origin_y, 2);
	zassert_equal(mock_display.line.end_x, 3);
	zassert_equal(mock_display.line.end_y, 4);
}

ZTEST(zbook_display, test_draw_rect_forwards_arguments)
{
	zbook_display_draw_rect(5, 6, 7, 8, true);

	zassert_equal(mock_display.rect.calls, 1);
	zassert_equal(mock_display.rect.top, 5);
	zassert_equal(mock_display.rect.left, 6);
	zassert_equal(mock_display.rect.height, 7);
	zassert_equal(mock_display.rect.width, 8);
	zassert_true(mock_display.rect.filled);

	zbook_display_draw_rect(5, 6, 7, 8, false);

	zassert_false(mock_display.rect.filled);
}

ZTEST(zbook_display, test_draw_circle_forwards_arguments)
{
	zbook_display_draw_circle(64, 32, 30);

	zassert_equal(mock_display.circle.calls, 1);
	zassert_equal(mock_display.circle.center_x, 64);
	zassert_equal(mock_display.circle.center_y, 32);
	zassert_equal(mock_display.circle.radius, 30);
}

ZTEST(zbook_display, test_draw_bitmap_forwards_arguments)
{
	static const uint8_t bmp[] = {0xAA, 0x55};

	zbook_display_draw_bitmap(bmp, 1, 2, 8, 2, 3);

	zassert_equal(mock_display.bitmap.calls, 1);
	zassert_equal_ptr(mock_display.bitmap.bmp, bmp);
	zassert_equal(mock_display.bitmap.top, 1);
	zassert_equal(mock_display.bitmap.left, 2);
	zassert_equal(mock_display.bitmap.width, 8);
	zassert_equal(mock_display.bitmap.height, 2);
	zassert_equal(mock_display.bitmap.scale, 3);
}

ZTEST(zbook_display, test_draw_bitmap_ignores_null)
{
	zbook_display_draw_bitmap(NULL, 0, 0, 8, 8, 1);

	zassert_equal(mock_display.bitmap.calls, 0);
}

ZTEST(zbook_display, test_print_forwards_arguments_and_result)
{
	const uint8_t *str = (const uint8_t *)"ZBook";
	uint16_t ret;

	ret = zbook_display_print(str, 10, 20, FONT_SIZE_7, 2);

	zassert_equal(ret, MOCK_DISPLAY_PRINT_RET);
	zassert_equal(mock_display.print.calls, 1);
	zassert_equal_ptr(mock_display.print.str, str);
	zassert_equal(mock_display.print.top, 10);
	zassert_equal(mock_display.print.left, 20);
	zassert_equal(mock_display.print.font, FONT_SIZE_7);
	zassert_equal(mock_display.print.scale, 2);
}

ZTEST(zbook_display, test_print_rejects_invalid_input)
{
	zassert_equal(zbook_display_print(NULL, 0, 0, FONT_SIZE_7, 1), 0);
	zassert_equal(zbook_display_print((const uint8_t *)"x", 0, 0, FONT_AMOUNT, 1), 0);
	zassert_equal(mock_display.print.calls, 0);
}

ZTEST(zbook_display, test_draw_mode_is_mapped_to_driver)
{
	zbook_display_set_draw_mode(ZBOOK_DISPLAY_PIXEL_WHITE);
	zassert_equal(mock_display.draw_mode.mode, DISPLAY_PIXEL_WHITE);

	zbook_display_set_draw_mode(ZBOOK_DISPLAY_PIXEL_BLACK);
	zassert_equal(mock_display.draw_mode.mode, DISPLAY_PIXEL_BLACK);

	zbook_display_set_draw_mode(ZBOOK_DISPLAY_PIXEL_INVERT);
	zassert_equal(mock_display.draw_mode.mode, DISPLAY_PIXEL_INVERT);

	zassert_equal(mock_display.draw_mode.calls, 3);
}

ZTEST(zbook_display, test_draw_mode_ignores_invalid_value)
{
	zbook_display_set_draw_mode((enum zbook_display_draw_mode)99);

	zassert_equal(mock_display.draw_mode.calls, 0);
}

ZTEST(zbook_display, test_flush_leaves_top_rows_to_the_user)
{
	zbook_display_set_draw_mode(ZBOOK_DISPLAY_PIXEL_INVERT);
	mock_display_reset();

	zbook_display_print((const uint8_t *)"Top", 0, 0, FONT_SIZE_7, 1);
	zbook_display_flush();

	zassert_equal(mock_display.print.calls, 1);
	zassert_equal(mock_display.print.top, 0);
	zassert_equal(mock_display.rect.calls, 0, "no status bar may be erased");
	zassert_equal(mock_display.bitmap.calls, 0, "no status bar icon may be drawn");
	zassert_equal(mock_display.draw_mode.calls, 0, "flush must not touch the draw mode");
	zassert_equal(mock_display.flush_calls, 1);
}
