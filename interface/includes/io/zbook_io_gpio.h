/*************************************************************************
 * @file zbook_io_gpio.h
 *
 * @brief GPIO backend of the Zbook header pins.
 *
 * @details configures a pin as a plain GPIO (direction, pull, initial/open-drain state)
 *			and drives or samples it. Used by the pin-function factory (zbook_pin.h);
 *
 * @note it does not track which function a pin holds.
 *
 * @author João Matheus Nascimento Dias (joao.dias@edge.ufal.br)
 * @version 0.1
 * @date 09/10/2026
 *
 * @copyright Centro de Inovação EDGE 2026. Todos os direitos reservados.
 *
 ***********************************************************************/
#ifndef ZBOOK_GPIO_FACTORY_H
#define ZBOOK_GPIO_FACTORY_H

#include "io/zbook_pin_cfg.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Per-pin GPIO state kept between init, read/write and deinit.
 */
struct zbook_gpio_ctx {
	enum zbook_pin_header_direction direction; /**< Configured direction. */
	bool open_drain; /**< HIGH releases the line instead of driving it. */
};

/**
 * @brief Configures a header pin as a GPIO.
 *
 * @param gpio[in] Header pin GPIO number.
 * @param cfg[in] Direction, pull and state.
 * @param ctx[out] Filled with the state later calls need.
 *
 * @retval 0 Success: the pin is configured as a GPIO.
 * @retval -EINVAL Error: gpio/cfg/ctx is NULL-or-invalid, or cfg has an out-of-range field.
 */
int zbook_io_gpio_init(enum zbook_pin_header gpio, const struct zbook_pin_cfg_gpio *cfg,
			    struct zbook_gpio_ctx *ctx);

/**
 * @brief Returns the pin to an unused (input, no pull, no function) state.
 *
 * @param gpio[in] Header pin GPIO number.
 */
void zbook_io_gpio_deinit(enum zbook_pin_header gpio);

/**
 * @brief Drives an output pin.
 *
 * @param gpio[in] Header pin GPIO number.
 * @param ctx[in] State from zbook_io_gpio_init().
 * @param buf[in] One byte: an enum zbook_pin_header_state.
 * @param len[in] Must be 1.
 *
 * @retval 0 Success: the pin is driven to the specified state.
 * @retval -EINVAL Error: @p len != 1 or the state is out of range.
 * @retval -ENOTSUP Error: The pin is configured as input.
 */
int zbook_io_gpio_write(enum zbook_pin_header gpio, const struct zbook_gpio_ctx *ctx,
			     const void *buf, size_t len);

/**
 * @brief Samples a pin.
 *
 * @param gpio[in] Header pin GPIO number.
 * @param ctx[in] State from zbook_io_gpio_init().
 * @param buf[out] One byte: ZBOOK_PIN_HEADER_STATE_LOW or _HIGH.
 * @param len[in] Must be 1.
 *
 * @retval 0 Success: the pin is sampled.
 * @retval -EINVAL Error: @p len != 1.
 */
int zbook_io_gpio_read(enum zbook_pin_header gpio, const struct zbook_gpio_ctx *ctx, void *buf,
			    size_t len);

#ifdef __cplusplus
}
#endif

#endif /* ZBOOK_GPIO_FACTORY_H */
