/************************************************************************
 * @file zbook_potentiometer.c
 *
 * @brief Implements the Interface for the Zbook potentiometer sensor input.
 * @author João Matheus Nascimento Dias (joao.dias@edge.ufal.br)
 * @version 0.2
 * @date 18/09/2026
 *
 * @copyright Centro de Inovação EDGE 2026. Todos os direitos reservados.
 *
 ***********************************************************************/

#include "sensors/zbook_potentiometer.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(zbook_potentiometer, CONFIG_POTENTIOMETER_LOG_LEVEL);

#define POTENTIOMETER_RESOLUTION 12
#define POTENTIOMETER_MAX_RAW    ((1 << POTENTIOMETER_RESOLUTION) - 1)

static const struct adc_channel_cfg potentiometer_channel_cfg =
	ADC_CHANNEL_CFG_DT(DT_NODELABEL(potentiometer));
static const struct device *adc_dev = DEVICE_DT_GET(DT_NODELABEL(adc));

int zbook_potentiometer_init(void)
{
	int ret;

	if (!device_is_ready(adc_dev)) {
		LOG_ERR("Potentiometer ADC device not ready");
		return -ENODEV;
	}

	ret = adc_channel_setup(adc_dev, &potentiometer_channel_cfg);
	if (ret) {
		LOG_ERR("Failed to setup potentiometer ADC channel (%d)", ret);
		return ret;
	}

	return 0;
}

int zbook_potentiometer_read(uint16_t *value)
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
		.channels = BIT(potentiometer_channel_cfg.channel_id),
		.resolution = POTENTIOMETER_RESOLUTION,
	};

	ret = adc_read(adc_dev, &sequence);
	if (ret) {
		LOG_ERR("ADC read failed (%d)", ret);
		return ret;
	}

	if (sample == 0) {
		*value = 0;
		return -EIO;
	}

	*value = (uint16_t)(((uint32_t)sample * 100) / POTENTIOMETER_MAX_RAW);

	return 0;
}