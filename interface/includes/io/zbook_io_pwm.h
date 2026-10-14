/*************************************************************************
 * @file zbook_io_pwm.h
 *
 * @brief PWM backend of the Zbook header pins.
 *
 * @details Routes a pin to its PWM slice/channel and generates a fixed
 *			frequency with an adjustable duty cycle. Used by the pin-function
 *			factory (zbook_pin.h).
 *
 * @note Each PWM slice drives two pins (channels A and B) with one shared
 *		 frequency: configuring the second pin of a slice changes the period
 *		 of the first one. It does not track which function a pin holds.
 *
 * @author João Matheus Nascimento Dias (joao.dias@edge.ufal.br)
 * @version 0.1
 * @date 13/10/2026
 *
 * @copyright Centro de Inovação EDGE 2026. Todos os direitos reservados.
 *
 ***********************************************************************/
#ifndef ZBOOK_IO_PWM_H
#define ZBOOK_IO_PWM_H

#include "io/zbook_pin_cfg.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Per-pin PWM state kept between init, write and deinit.
 */
struct zbook_pwm_ctx {
	enum zbook_pin_header_pwm_polarity polarity; /**< Configured polarity. */
	uint16_t wrap;                               /**< Counter top: period = wrap + 1 ticks. */
};

/**
 * @brief Configures a header pin as a PWM output and starts it.
 *
 * @param gpio[in] Header pin GPIO number.
 * @param cfg[in] Frequency, duty cycle and polarity.
 * @param ctx[out] Filled with the state later calls need.
 *
 * @retval 0 Success: the pin outputs the requested PWM signal.
 * @retval -EINVAL Error: NULL argument, zero frequency, duty above 100 or a
 *         frequency the slice cannot generate.
 */
int zbook_io_pwm_init(enum zbook_pin_header gpio, const struct zbook_pin_cfg_pwm *cfg,
		      struct zbook_pwm_ctx *ctx);

/**
 * @brief Stops the pin's PWM output and returns the pin to an unused state.
 *
 * @param gpio[in] Header pin GPIO number.
 */
void zbook_io_pwm_deinit(enum zbook_pin_header gpio);

/**
 * @brief Changes the duty cycle of a running PWM pin.
 *
 * @param gpio[in] Header pin GPIO number.
 * @param ctx[in] State from zbook_io_pwm_init().
 * @param buf[in] One byte: duty cycle in percent (0-100).
 * @param len[in] Must be 1.
 *
 * @retval 0 Success: the new duty cycle is applied.
 * @retval -EINVAL Error: @p len != 1 or the duty cycle is above 100.
 */
int zbook_io_pwm_write(enum zbook_pin_header gpio, const struct zbook_pwm_ctx *ctx, const void *buf,
		       size_t len);

#ifdef __cplusplus
}
#endif

#endif /* ZBOOK_IO_PWM_H */
