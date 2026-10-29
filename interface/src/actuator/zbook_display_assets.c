/************************************************************************
 * @file zbook_display_assets.c
 *
 * @brief Implementation of the Zbook Display actuator interface with assets.
 * @author João Matheus Nascimento Dias (joao.dias@edge.ufal.br)
 * @version 0.1
 * @date 23/10/2026
 *
 * @copyright Centro de Inovação EDGE 2026. Todos os direitos reservados.
 *
 ***********************************************************************/
#include <stddef.h>

#include "actuator/assets/assets.h"
#include "actuator/zbook_display.h"
#include "display.h"

#define STATUS_BAR_ICON_GAP     2
#define STATUS_BAR_RIGHT_MARGIN 2

static struct {
	struct {
		const uint8_t *off;
		const uint8_t *on;
		uint8_t width;
		uint8_t height;
	} icons[ZBOOK_DISPLAY_ICON_AMOUNT];
	enum zbook_display_icon_state icon_states[ZBOOK_DISPLAY_ICON_AMOUNT];
	const uint8_t *const wifi_levels[ZBOOK_DISPLAY_WIFI_LEVEL_MAX + 1];
	uint8_t wifi_level;
} self = {
	.icons =
		{
			[ZBOOK_DISPLAY_ICON_WIFI] = {zbook_wifi_off, zbook_wifi, ZBOOK_WIFI_W,
						     ZBOOK_WIFI_H},
			[ZBOOK_DISPLAY_ICON_BLUETOOTH] = {zbook_bt_off, zbook_bt, ZBOOK_BT_W,
							  ZBOOK_BT_H},
			[ZBOOK_DISPLAY_ICON_SD] = {zbook_sd_off, zbook_sd, ZBOOK_SD_W, ZBOOK_SD_H},
			[ZBOOK_DISPLAY_ICON_ERROR] = {NULL, zbook_error, ZBOOK_ERROR_W,
						      ZBOOK_ERROR_H},
		},
	.wifi_levels =
		{
			zbook_wifi_level_0,
			zbook_wifi_level_1,
			zbook_wifi_level_2,
			zbook_wifi_level_3,
		},
	.icon_states =
		{
			[ZBOOK_DISPLAY_ICON_WIFI] = ZBOOK_DISPLAY_ICON_OFF,
			[ZBOOK_DISPLAY_ICON_BLUETOOTH] = ZBOOK_DISPLAY_ICON_OFF,
			[ZBOOK_DISPLAY_ICON_SD] = ZBOOK_DISPLAY_ICON_OFF,
			[ZBOOK_DISPLAY_ICON_ERROR] = ZBOOK_DISPLAY_ICON_OFF,
		},
	.wifi_level = ZBOOK_DISPLAY_WIFI_LEVEL_MAX,
};

static const uint8_t *icon_bitmap(enum zbook_display_icon icon)
{
	switch (self.icon_states[icon]) {
	case ZBOOK_DISPLAY_ICON_ON:
		return (icon == ZBOOK_DISPLAY_ICON_WIFI) ? self.wifi_levels[self.wifi_level]
							 : self.icons[icon].on;
	case ZBOOK_DISPLAY_ICON_OFF:
		return self.icons[icon].off;
	default:
		return NULL;
	}
}

void zbook_display_draw_status_bar(void)
{
	uint8_t left = SCR_W - STATUS_BAR_RIGHT_MARGIN;

	display_set_draw_mode(DISPLAY_PIXEL_BLACK);
	display_draw_rect(0, 0, ZBOOK_DISPLAY_STATUS_BAR_H - 1, SCR_W - 1, true);
	display_set_draw_mode(DISPLAY_PIXEL_WHITE);

	for (unsigned int i = 0; i < ZBOOK_DISPLAY_ICON_AMOUNT; i++) {
		const uint8_t *bmp = icon_bitmap(i);

		if (bmp == NULL) {
			continue;
		}

		left -= self.icons[i].width;
		display_draw_bitmap(bmp, 0, left, self.icons[i].width, self.icons[i].height, 1);
		left -= STATUS_BAR_ICON_GAP;
	}
}

void zbook_display_set_icon(enum zbook_display_icon icon, enum zbook_display_icon_state state)
{
	if (((unsigned int)icon >= ZBOOK_DISPLAY_ICON_AMOUNT) ||
	    ((unsigned int)state > ZBOOK_DISPLAY_ICON_ON)) {
		return;
	}

	self.icon_states[icon] = state;
}

void zbook_display_set_wifi_level(uint8_t level)
{
	if (level > ZBOOK_DISPLAY_WIFI_LEVEL_MAX) {
		return;
	}

	self.wifi_level = level;
}
