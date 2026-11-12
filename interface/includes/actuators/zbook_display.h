/************************************************************************
 * @file zbook_display.h
 *
 * @brief Defines the Interface for the Zbook Display actuator.
 * @author João Matheus Nascimento Dias (joao.dias@edge.ufal.br)
 * @version 0.1
 * @date 11/09/2026
 *
 * @copyright Centro de Inovação EDGE 2026. Todos os direitos reservados.
 *
 ***********************************************************************/
#ifndef ZBOOK_DISPLAY_H
#define ZBOOK_DISPLAY_H

#include "fonts.h"

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Width of the visible area of the display, in pixels.
 *
 */
#define ZBOOK_DISPLAY_VISIBLE_W 126

/**
 * @brief Height of the visible area of the display, in pixels.
 *
 */
#define ZBOOK_DISPLAY_VISIBLE_H 64

/**
 * @brief Enumera os modos de desenho.
 */
enum zbook_display_draw_mode {
	ZBOOK_DISPLAY_PIXEL_WHITE = 0, /**< Pixel on overwrites the RAM. */
	ZBOOK_DISPLAY_PIXEL_BLACK,     /**< Pixel on erases the RAM. */
	ZBOOK_DISPLAY_PIXEL_INVERT,    /**< Pixel on inverts the RAM. */
};

#if CONFIG_ZBOOK_INTERFACE_ACTUATOR_DISPLAY_STATUS_BAR

/**
 * @brief Height of the status bar, in pixels. Matches the yellow zone of the panel.
 */
#define ZBOOK_DISPLAY_STATUS_BAR_H 16

/**
 * @brief Highest Wi-Fi signal level, drawn with every arc lit.
 */
#define ZBOOK_DISPLAY_WIFI_LEVEL_MAX 3

/**
 * @brief Enumerates the status bar icons. The order sets the slots, from right to left.
 */
enum zbook_display_icon {
	ZBOOK_DISPLAY_ICON_WIFI = 0,  /**< Wi-Fi icon. ON draws the signal level. */
	ZBOOK_DISPLAY_ICON_BLUETOOTH, /**< Bluetooth icon. */
	ZBOOK_DISPLAY_ICON_SD,        /**< MicroSD icon. ON = card inserted, OFF = no card. */
	ZBOOK_DISPLAY_ICON_ERROR,     /**< Error icon. Has no OFF drawing, OFF hides it. */
	ZBOOK_DISPLAY_ICON_AMOUNT,    /**< Amount of icons. */
};

/**
 * @brief Enumerates the states of a status bar icon.
 */
enum zbook_display_icon_state {
	ZBOOK_DISPLAY_ICON_HIDDEN = 0, /**< Icon is not drawn, the others take its slot. */
	ZBOOK_DISPLAY_ICON_OFF,        /**< Icon is drawn slashed. */
	ZBOOK_DISPLAY_ICON_ON,         /**< Icon is drawn normally. */
};

/**
 * @brief Set the state of a status bar icon. Applied on the next flush.
 *
 * @param icon Icon to be set.
 * @param state State to be set.
 */
void zbook_display_set_icon(enum zbook_display_icon icon, enum zbook_display_icon_state state);

/**
 * @brief Set the signal level drawn by the Wi-Fi icon while it is ON. Applied on the next flush.
 *
 * @param level Signal level, from 0 up to ZBOOK_DISPLAY_WIFI_LEVEL_MAX. Higher values are ignored.
 */
void zbook_display_set_wifi_level(uint8_t level);

/**
 * @brief Erase the status bar zone and draw the visible icons into the vRAM, right aligned.
 * Hidden icons give their slot to the next ones. Already called by zbook_display_flush().
 *
 * @note Leaves the draw mode as ZBOOK_DISPLAY_PIXEL_WHITE.
 */
void zbook_display_draw_status_bar(void);

#endif /* CONFIG_ZBOOK_INTERFACE_ACTUATOR_DISPLAY_STATUS_BAR */

/**
 * @brief Inicializy the display with default settings.
 *
 */
void zbook_display_init(void);

/**
 * @brief Disable the display.
 */
void zbook_display_disable(void);

/**
 * @brief Enable the display.
 */
void zbook_display_enable(void);

/**
 * @brief Clear the buffer VRAM of the display.
 */
void zbook_display_clear(void);

/**
 * @brief Send the vRAM to the display. When the status bar is enabled, it is redrawn
 * on top of the vRAM first.
 */
void zbook_display_flush(void);

/**
 * @brief Draw a line.
 *
 * @param origin_x x of origin.
 * @param origin_y y of origin.
 * @param end_x x of destination.
 * @param end_y y of destination.
 */
void zbook_display_draw_line(uint8_t origin_x, uint8_t origin_y, uint8_t end_x, uint8_t end_y);

/**
 * @brief Draw a rectangle.
 *
 * @param top y of the top edge.
 * @param left x of the left edge.
 * @param height Height of the rectangle.
 * @param width Width of the rectangle.
 * @param filled If true, the rectangle will be filled.
 */
void zbook_display_draw_rect(uint8_t top, uint8_t left, uint8_t height, uint8_t width, bool filled);

/**
 * @brief Draw a circle.
 *
 * @param center_x x of center of the circle.
 * @param center_y y of center of the circle.
 * @param radius Radius of the circle.
 */
void zbook_display_draw_circle(uint8_t center_x, uint8_t center_y, uint8_t radius);

/**
 * @brief Draw a bitmap.
 *
 * @param[in] bmp Reference of the bitmap.
 * @param top x left of the bitmap.
 * @param left y upper of the bitmap.
 * @param width Horizontal size of the bitmap.
 * @param height Vertical size of the bitmap.
 * @param scale Scale factor for the bitmap.
 */
void zbook_display_draw_bitmap(const uint8_t *bmp, uint8_t top, uint8_t left, uint8_t width,
			       uint8_t height, uint8_t scale);

/**
 * @brief Print a string.
 *
 * @param[in] str Reference to a string.
 * @param top y upper of the string.
 * @param left x left of the string.
 * @param font Font to be used.
 * @param scale Scale factor for the text.
 * @return Position of the right corner of the string, in pixels.
 */
uint16_t zbook_display_print(const uint8_t *str, uint8_t top, uint8_t left, enum font_sizes font,
			     uint8_t scale);

/**
 * @brief Settings the mode of drawing of the display.
 *
 * @param mode Mode to be set.
 */
void zbook_display_set_draw_mode(enum zbook_display_draw_mode mode);

#endif /* ZBOOK_DISPLAY_H */