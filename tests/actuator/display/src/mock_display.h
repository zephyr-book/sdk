/************************************************************************
 * @file mock_display.h
 *
 * @brief Mock of the deps/display driver, records every call it gets.
 * @author João Matheus Nascimento Dias (joao.dias@edge.ufal.br)
 * @version 0.1
 * @date 27/10/2026
 *
 * @copyright Centro de Inovação EDGE 2026. Todos os direitos reservados.
 *
 ***********************************************************************/
#ifndef MOCK_DISPLAY_H
#define MOCK_DISPLAY_H

#include "display.h"

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Value returned by the mocked display_print().
 */
#define MOCK_DISPLAY_PRINT_RET 42

/**
 * @brief Calls recorded by the mock. Each field set holds the call count, the
 * order of the last call (shared sequence) and the last arguments.
 */
struct mock_display {
	uint32_t seq; /**< Shared call sequence. */

	struct {
		uint32_t calls;
		uint32_t order;
		bool is_dcdc_dependent;
	} init;

	struct {
		uint32_t calls;
		uint32_t order;
		uint8_t amount;
	} contrast;

	uint32_t disable_calls;
	uint32_t enable_calls;
	uint32_t clear_calls;
	uint32_t flush_calls;

	struct {
		uint32_t calls;
		uint8_t origin_x;
		uint8_t origin_y;
		uint8_t end_x;
		uint8_t end_y;
	} line;

	struct {
		uint32_t calls;
		uint8_t top;
		uint8_t left;
		uint8_t height;
		uint8_t width;
		bool filled;
	} rect;

	struct {
		uint32_t calls;
		uint8_t center_x;
		uint8_t center_y;
		uint8_t radius;
	} circle;

	struct {
		uint32_t calls;
		const uint8_t *bmp;
		uint8_t top;
		uint8_t left;
		uint8_t width;
		uint8_t height;
		uint8_t scale;
	} bitmap;

	struct {
		uint32_t calls;
		const uint8_t *str;
		uint8_t top;
		uint8_t left;
		enum font_sizes font;
		uint8_t scale;
	} print;

	struct {
		uint32_t calls;
		enum display_draw_mode mode;
	} draw_mode;
};

/**
 * @brief Calls recorded since the last mock_display_reset().
 */
extern struct mock_display mock_display;

/**
 * @brief Clears every recorded call.
 */
void mock_display_reset(void);

#endif /* MOCK_DISPLAY_H */
