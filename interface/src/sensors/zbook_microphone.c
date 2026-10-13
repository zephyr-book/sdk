/*******************************************************************
 * @file zbook_microphone.c
 *
 * @brief Implements the Interface for the Zbook microphone sensor input.
 * @author Matheus Macário dos Santos (matheus.macario@edge.ufal.br)
 * @version 0.1
 * @date 23/09/2026
 *
 * @copyright Copyright (c) Centro de Inovacao EDGE - 2026
 *
 *******************************************************************/

#include "sensors/zbook_microphone.h"

#ifdef CONFIG_ZBOOK_INTERFACE_SENSORS_MICROPHONE

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(zbook_microphone, CONFIG_ZBOOK_MICROPHONE_LOG_LEVEL);

#define MICROPHONE_RESOLUTION 12

static const struct adc_channel_cfg microphone_channel_cfg =
	ADC_CHANNEL_CFG_DT(DT_NODELABEL(microphone_adc));
static const struct device *adc_dev = DEVICE_DT_GET(DT_NODELABEL(adc));

int zbook_microphone_init(void)
{
	int ret;

	if (!device_is_ready(adc_dev)) {
		LOG_ERR("Microphone ADC device not ready");
		return -ENODEV;
	}

	ret = adc_channel_setup(adc_dev, &microphone_channel_cfg);
	if (ret) {
		LOG_ERR("Failed to setup microphone ADC channel (%d)", ret);
		return ret;
	}

	return 0;
}

int zbook_microphone_read(uint16_t *value)
{
	if (value == NULL) {
		LOG_ERR("Value pointer is NULL");
		return -EINVAL;
	}

	int ret;
	uint16_t sample = 0;

	struct adc_sequence sequence = {
		.buffer = &sample,
		.buffer_size = sizeof(sample),
		.calibrate = true,
		.channels = BIT(microphone_channel_cfg.channel_id),
		.resolution = MICROPHONE_RESOLUTION,
	};

	ret = adc_read(adc_dev, &sequence);
	if (ret) {
		LOG_ERR("ADC read failed (%d)", ret);
		return ret;
	}

	*value = sample;

	return 0;
}

#endif /* CONFIG_ZBOOK_INTERFACE_SENSORS_MICROPHONE */
