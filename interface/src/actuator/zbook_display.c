/************************************************************************
 * @file zbook_display.c
 *
 * @brief Implementation of the Zbook Display actuator interface.
 * @author João Matheus Nascimento Dias (joao.dias@edge.ufal.br)
 * @version 0.1
 * @date 23/10/2026
 *
 * @copyright Centro de Inovação EDGE 2026. Todos os direitos reservados.
 *
 ***********************************************************************/

#include <zephyr/sys/util.h>

#include "actuator/zbook_display.h"
#include "display.h"

#if CONFIG_ZBOOK_INTERFACE_ACTUATORS_DISPLAY_ASSETS
#include <zephyr/kernel.h>

#include "actuator/assets/assets.h"

#define LOGO_TIME_MS 3000
#endif

#define SSD1306_MODE                                                                               \
	(IS_ENABLED(CONFIG_OLED_DISPLAY_I2C) &&                                                    \
	 DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_display), solomon_ssd1306))

#if SSD1306_MODE

#include <zephyr/drivers/i2c.h>

#define SSD1306_UNWRITTEN_COLS 2

static const struct i2c_dt_spec display_i2c = I2C_DT_SPEC_GET(DT_CHOSEN(zephyr_display));

enum zbook_display_ssd1306_cmd {
	ZBOOK_DISPLAY_SSD1306_ADDRESSING_PAGE = 0x02,
	ZBOOK_DISPLAY_SSD1306_CMD_CHARGE_PUMP = 0x8D,
	ZBOOK_DISPLAY_SSD1306_CHARGE_PUMP_ON = 0x14,
	ZBOOK_DISPLAY_SSD1306_CHARGE_PUMP_OFF = 0x10,
	ZBOOK_DISPLAY_SSD1306_CMD_ADDRESSING_MODE = 0x20,
	ZBOOK_DISPLAY_SSD1306_CMD_DISPLAY_ON = 0xAF,
	ZBOOK_DISPLAY_SSD1306_CMD_COL_LOW = 0x00,
	ZBOOK_DISPLAY_SSD1306_CMD_COL_HIGH = 0x10,
	ZBOOK_DISPLAY_SSD1306_CMD_PAGE_START = 0xB0,
};

static void ssd1306_init(void)
{
	const uint8_t cmds[] = {
		CONFIG_DISPLAY_I2C_CTRL_CMD,
		ZBOOK_DISPLAY_SSD1306_CMD_CHARGE_PUMP,
		IS_ENABLED(CONFIG_ZBOOK_INTERFACE_DISPLAY_DCDC)
			? ZBOOK_DISPLAY_SSD1306_CHARGE_PUMP_ON
			: ZBOOK_DISPLAY_SSD1306_CHARGE_PUMP_OFF,
		ZBOOK_DISPLAY_SSD1306_CMD_ADDRESSING_MODE,
		ZBOOK_DISPLAY_SSD1306_ADDRESSING_PAGE,
		ZBOOK_DISPLAY_SSD1306_CMD_DISPLAY_ON,
	};

	(void)i2c_write_dt(&display_i2c, cmds, sizeof(cmds));
}

static void ssd1306_clear_unwritten_cols(void)
{
	const uint8_t data[1 + SSD1306_UNWRITTEN_COLS] = {CONFIG_DISPLAY_I2C_CTRL_DATA};

	for (uint8_t page = 0; page < (SCR_H / 8); page++) {
		const uint8_t cmds[] = {
			CONFIG_DISPLAY_I2C_CTRL_CMD,
			ZBOOK_DISPLAY_SSD1306_CMD_PAGE_START + page,
			ZBOOK_DISPLAY_SSD1306_CMD_COL_LOW,
			ZBOOK_DISPLAY_SSD1306_CMD_COL_HIGH,
		};

		(void)i2c_write_dt(&display_i2c, cmds, sizeof(cmds));
		(void)i2c_write_dt(&display_i2c, data, sizeof(data));
	}
}
#endif /* SSD1306_MODE */

static const enum display_draw_mode draw_mode_map[] = {
	[ZBOOK_DISPLAY_PIXEL_WHITE] = DISPLAY_PIXEL_WHITE,
	[ZBOOK_DISPLAY_PIXEL_BLACK] = DISPLAY_PIXEL_BLACK,
	[ZBOOK_DISPLAY_PIXEL_INVERT] = DISPLAY_PIXEL_INVERT,
};

#if CONFIG_ZBOOK_INTERFACE_ACTUATOR_DISPLAY_STATUS_BAR
static enum zbook_display_draw_mode current_mode = ZBOOK_DISPLAY_PIXEL_WHITE;
#endif

static void flush_vram(void)
{
	display_flush();

#if SSD1306_MODE
	ssd1306_clear_unwritten_cols();
#endif
}

#if CONFIG_ZBOOK_INTERFACE_ACTUATORS_DISPLAY_ASSETS
static void draw_logo(void)
{
	const uint8_t top = (ZBOOK_DISPLAY_VISIBLE_H - ZBOOK_LOGO_H) / 2;
	const uint8_t left = (ZBOOK_DISPLAY_VISIBLE_W - ZBOOK_LOGO_W) / 2;

	display_clear();
	display_draw_bitmap(zbook_logo, top, left, ZBOOK_LOGO_W, ZBOOK_LOGO_H, 1);
	display_print((const uint8_t *)"P@2", top + 10, left + ((ZBOOK_LOGO_W - 17) / 2),
		      FONT_SIZE_7, 1);

	flush_vram();

	k_msleep(LOGO_TIME_MS);

	display_clear();
}
#endif

void zbook_display_init(void)
{
	display_init(IS_ENABLED(CONFIG_ZBOOK_INTERFACE_DISPLAY_DCDC));

#if SSD1306_MODE
	ssd1306_init();
#endif

#if !CONFIG_DISPLAY_VIRTUAL
	display_set_contrast(CONFIG_ZBOOK_INTERFACE_DISPLAY_INIT_CONTRAST);
#endif

#if CONFIG_ZBOOK_INTERFACE_ACTUATORS_DISPLAY_ASSETS
	draw_logo();
#endif
}

void zbook_display_disable(void)
{
	display_disable();
}

void zbook_display_enable(void)
{
	display_enable();
}

void zbook_display_clear(void)
{
	display_clear();
}

void zbook_display_flush(void)
{
#if CONFIG_ZBOOK_INTERFACE_ACTUATOR_DISPLAY_STATUS_BAR
	zbook_display_draw_status_bar();
	display_set_draw_mode(draw_mode_map[current_mode]);
#endif

	flush_vram();
}

void zbook_display_draw_line(uint8_t origin_x, uint8_t origin_y, uint8_t end_x, uint8_t end_y)
{
	display_draw_line(origin_x, origin_y, end_x, end_y);
}

void zbook_display_draw_rect(uint8_t top, uint8_t left, uint8_t height, uint8_t width, bool filled)
{
	display_draw_rect(top, left, height, width, filled);
}

void zbook_display_draw_circle(uint8_t center_x, uint8_t center_y, uint8_t radius)
{
	display_draw_circle(center_x, center_y, radius);
}

void zbook_display_draw_bitmap(const uint8_t *bmp, uint8_t top, uint8_t left, uint8_t width,
			       uint8_t height, uint8_t scale)
{
	if (bmp == NULL) {
		return;
	}

	display_draw_bitmap(bmp, top, left, width, height, scale);
}

uint16_t zbook_display_print(const uint8_t *str, uint8_t top, uint8_t left, enum font_sizes font,
			     uint8_t scale)
{
	if ((str == NULL) || (font >= FONT_AMOUNT)) {
		return 0;
	}

	return display_print(str, top, left, font, scale);
}

void zbook_display_set_draw_mode(enum zbook_display_draw_mode mode)
{
	if ((unsigned int)mode >= ARRAY_SIZE(draw_mode_map)) {
		return;
	}

#if CONFIG_ZBOOK_INTERFACE_ACTUATOR_DISPLAY_STATUS_BAR
	current_mode = mode;
#endif

	display_set_draw_mode(draw_mode_map[mode]);
}
