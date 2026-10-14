/*************************************************************************
 * @file zbook_io_adc.c
 *
 * @brief Implementation of the Zbook ADC backend.
 * @author João Matheus Nascimento Dias (joao.dias@edge.ufal.br)
 * @version 0.1
 * @date 13/10/2026
 *
 * @copyright Centro de Inovação EDGE 2026. Todos os direitos reservados.
 *
 ***********************************************************************/

#include "io/zbook_io_adc.h"

#include <errno.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/sys/util.h>

#include <hardware/gpio.h>

#define ADC_NATIVE_BITS 12
#define ADC_FIRST_GPIO  40
#define ADC_LAST_GPIO   47

static const struct device *const adc_dev = DEVICE_DT_GET(DT_NODELABEL(adc));

static void adc_pin_release(enum zbook_pin_header gpio)
{
	gpio_set_dir(gpio, GPIO_IN);
	gpio_disable_pulls(gpio);
	gpio_set_function(gpio, GPIO_FUNC_NULL);
}

int zbook_io_adc_init(enum zbook_pin_header gpio, const struct zbook_pin_cfg_adc *cfg,
		      struct zbook_adc_ctx *ctx)
{
	if (cfg == NULL || ctx == NULL || cfg->resolution > ADC_NATIVE_BITS) {
		return -EINVAL;
	}

	if (gpio < ADC_FIRST_GPIO || gpio > ADC_LAST_GPIO) {
		return -ENOTSUP;
	}

	if (!device_is_ready(adc_dev)) {
		return -ENODEV;
	}

	struct adc_channel_cfg channel_cfg = {
		.gain = ADC_GAIN_1,
		.reference = ADC_REF_INTERNAL,
		.acquisition_time = ADC_ACQ_TIME_DEFAULT,
		.channel_id = gpio - ADC_FIRST_GPIO,
	};
	int ret = adc_channel_setup(adc_dev, &channel_cfg);

	if (ret != 0) {
		return ret;
	}

	/* Analog pad: no function, no pulls, digital input buffer off. */
	gpio_set_function(gpio, GPIO_FUNC_NULL);
	gpio_disable_pulls(gpio);
	gpio_set_input_enabled(gpio, false);

	ctx->channel = channel_cfg.channel_id;
	ctx->resolution = cfg->resolution != 0 ? cfg->resolution : ADC_NATIVE_BITS;

	return 0;
}

void zbook_io_adc_deinit(enum zbook_pin_header gpio)
{
	adc_pin_release(gpio);
}

int zbook_io_adc_read(enum zbook_pin_header gpio, const struct zbook_adc_ctx *ctx, void *buf,
		      size_t len)
{
	ARG_UNUSED(gpio);

	if (len != sizeof(uint16_t)) {
		return -EINVAL;
	}

	uint16_t sample;
	const struct adc_sequence sequence = {
		.channels = BIT(ctx->channel),
		.buffer = &sample,
		.buffer_size = sizeof(sample),
		.resolution = ADC_NATIVE_BITS,
	};
	int ret = adc_read(adc_dev, &sequence);

	if (ret != 0) {
		return ret;
	}

	sample >>= ADC_NATIVE_BITS - ctx->resolution;
	memcpy(buf, &sample, sizeof(sample));

	return 0;
}
