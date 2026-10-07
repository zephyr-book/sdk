/************************************************************************
 * @file zbook_ldr.c
 *
 * @brief Implements the Interface for the Zbook light sensor input.
 * @author João Matheus Nascimento Dias (joao.dias@edge.ufal.br)
 * @version 0.1
 * @date 17/09/2026
 *
 * @copyright Centro de Inovação EDGE 2026. Todos os direitos reservados.
 *
 ***********************************************************************/

#include "sensors/zbook_ldr.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(zbook_ldr, CONFIG_LDR_LOG_LEVEL);

#define LDR_RESOLUTION DT_PROP(DT_NODELABEL(ldr_adc), zephyr_resolution)
#define LDR_MAX_RAW    ((1 << LDR_RESOLUTION) - 1)

static const struct adc_channel_cfg ldr_channel_cfg = ADC_CHANNEL_CFG_DT(DT_NODELABEL(ldr_adc));
static const struct device *adc_dev = DEVICE_DT_GET(DT_NODELABEL(adc));

static int ldr_sample(uint16_t *sample, bool calibrate)
{
	struct adc_sequence sequence = {
		.buffer = sample,
		.buffer_size = sizeof(*sample),
		.calibrate = calibrate,
		.channels = BIT(ldr_channel_cfg.channel_id),
		.resolution = LDR_RESOLUTION,
	};

	return adc_read(adc_dev, &sequence);
}

int zbook_ldr_init(void)
{
	int ret;

	if (!device_is_ready(adc_dev)) {
		LOG_ERR("LDR ADC device not ready");
		return -ENODEV;
	}

	ret = adc_channel_setup(adc_dev, &ldr_channel_cfg);
	if (ret) {
		LOG_ERR("Failed to setup LDR ADC channel (%d)", ret);
		return ret;
	}

	uint16_t dummy;

	ret = ldr_sample(&dummy, true);
	if (ret) {
		LOG_ERR("LDR ADC calibration failed (%d)", ret);
		return -EIO;
	}

	return 0;
}

int zbook_ldr_read(uint16_t *value)
{
	if (value == NULL) {
		LOG_ERR("Value pointer is NULL");
		return -EINVAL;
	}

	int ret;
	uint16_t sample = 0;

	ret = ldr_sample(&sample, false);
	if (ret) {
		LOG_ERR("ADC read failed (%d)", ret);
		return ret;
	}

	if (sample == 0) {
		*value = 0;
		return -EIO;
	}

	*value = (uint16_t)(((uint32_t)sample * 100) / LDR_MAX_RAW);

	return 0;
}