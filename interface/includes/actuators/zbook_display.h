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
 * @brief Send the vRAM to the display.
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