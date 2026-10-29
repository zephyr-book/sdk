/************************************************************************
 * @file main.c
 *
 * @brief ZBook Display status bar sample: shows the ZBook logo, then cycles the status bar
 * icons and a menu mirroring them.
 * @author João Matheus Nascimento Dias (joao.dias@edge.ufal.br)
 * @version 0.1
 * @date 27/10/2026
 *
 * @copyright Centro de Inovação EDGE 2026. Todos os direitos reservados.
 *
 ***********************************************************************/

#include "actuator/zbook_display.h"
#include "actuator/assets/assets.h"

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>


#define STEP_TIME_MS 1500
#define MENU_TOP (ZBOOK_DISPLAY_STATUS_BAR_H + 3)
#define MENU_ROW_H 11

struct step {
	enum zbook_display_icon_state states[ZBOOK_DISPLAY_ICON_AMOUNT]; /**< Icon states. */
	uint8_t wifi_level;                                              /**< Wi-Fi level. */
	uint8_t cursor;                                                  /**< Selected row. */
};


#define H ZBOOK_DISPLAY_ICON_HIDDEN
#define F ZBOOK_DISPLAY_ICON_OFF
#define N ZBOOK_DISPLAY_ICON_ON

static const struct step steps[] = {
	{{H, H, H, H}, 3, 0}, {{N, H, H, H}, 0, 0}, {{N, H, H, H}, 1, 0}, {{N, H, H, H}, 2, 0},
	{{N, H, H, H}, 3, 0}, {{N, N, H, H}, 3, 1}, {{N, N, F, H}, 3, 2}, {{N, N, N, H}, 3, 2},
	{{N, N, N, N}, 3, 3}, {{F, N, N, H}, 3, 0}, {{F, F, F, H}, 3, 1},
};

static const char *const labels[ZBOOK_DISPLAY_ICON_AMOUNT] = {
	[ZBOOK_DISPLAY_ICON_WIFI] = "Wi-Fi",
	[ZBOOK_DISPLAY_ICON_BLUETOOTH] = "Bluetooth",
	[ZBOOK_DISPLAY_ICON_SD] = "MicroSD",
	[ZBOOK_DISPLAY_ICON_ERROR] = "Error",
};

static void draw_step(const struct step *s)
{
	zbook_display_clear();
	zbook_display_set_wifi_level(s->wifi_level);

	for (uint8_t i = 0; i < ZBOOK_DISPLAY_ICON_AMOUNT; i++) {
		const uint8_t top = MENU_TOP + (i * MENU_ROW_H);

		zbook_display_set_icon(i, s->states[i]);

		if (i == s->cursor) {
			zbook_display_draw_bitmap(zbook_cursor, top, 0, ZBOOK_CURSOR_W,
						  ZBOOK_CURSOR_H, 1);
		}
		zbook_display_draw_bitmap((s->states[i] == ZBOOK_DISPLAY_ICON_ON)
						  ? zbook_checkbox_on
						  : zbook_checkbox_off,
					  top, 10, ZBOOK_CHECKBOX_ON_W, ZBOOK_CHECKBOX_ON_H, 1);
		zbook_display_print((const uint8_t *)labels[i], top, 22, FONT_SIZE_7, 1);
	}

	zbook_display_flush();
}

int main(void)
{
	zbook_display_init();

	while (true) {
		for (size_t i = 0; i < ARRAY_SIZE(steps); i++) {
			draw_step(&steps[i]);
			k_msleep(STEP_TIME_MS);
		}
	}

	return 0;
}
