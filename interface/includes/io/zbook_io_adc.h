/*************************************************************************
 * @file zbook_io_adc.h
 *
 * @brief ADC backend of the Zbook header pins.
 *
 * @details Routes a pin to its ADC channel (GPIO40..47 -> channel 0..7) and
 *			samples it on demand. Used by the pin-function factory
 *			(zbook_pin.h).
 *
 * @note it does not track which function a pin holds.
 *
 * @author João Matheus Nascimento Dias (joao.dias@edge.ufal.br)
 * @version 0.1
 * @date 13/10/2026
 *
 * @copyright Centro de Inovação EDGE 2026. Todos os direitos reservados.
 *
 ***********************************************************************/
#ifndef ZBOOK_IO_ADC_H
#define ZBOOK_IO_ADC_H

#include "io/zbook_pin_cfg.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Per-pin ADC state kept between init, read and deinit.
 */
struct zbook_adc_ctx {
	uint8_t channel;    /**< ADC channel wired to the pin. */
	uint8_t resolution; /**< Bits returned by read (1-12). */
};

/**
 * @brief Configures a header pin as an ADC input.
 *
 * @param gpio[in] Header pin GPIO number.
 * @param cfg[in] Resolution in bits (0 selects the native 12).
 * @param ctx[out] Filled with the state later calls need.
 *
 * @retval 0 Success: the pin is an ADC input.
 * @retval -EINVAL Error: NULL argument or resolution above 12.
 * @retval -ENOTSUP Error: the pin has no ADC channel.
 * @retval -ENODEV Error: the ADC device is not ready.
 */
int zbook_io_adc_init(enum zbook_pin_header gpio, const struct zbook_pin_cfg_adc *cfg,
		      struct zbook_adc_ctx *ctx);

/**
 * @brief Returns the pin to an unused (input, no pull, no function) state.
 *
 * @param gpio[in] Header pin GPIO number.
 */
void zbook_io_adc_deinit(enum zbook_pin_header gpio);

/**
 * @brief Samples the pin.
 *
 * @param gpio[in] Header pin GPIO number.
 * @param ctx[in] State from zbook_io_adc_init().
 * @param buf[out] One uint16_t: the raw sample, scaled to the configured resolution.
 * @param len[in] Must be sizeof(uint16_t).
 *
 * @retval 0 Success: the pin is sampled.
 * @retval -EINVAL Error: @p len != sizeof(uint16_t).
 * @retval other Error: reported by the ADC driver.
 */
int zbook_io_adc_read(enum zbook_pin_header gpio, const struct zbook_adc_ctx *ctx, void *buf,
		      size_t len);

#ifdef __cplusplus
}
#endif

#endif /* ZBOOK_IO_ADC_H */
