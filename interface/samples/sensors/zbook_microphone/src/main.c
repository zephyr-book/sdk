/*******************************************************************
 * @file main.c
 *
 * @brief Sample application for the Zbook microphone sensor input.
 * @author Matheus Macário dos Santos (matheus.macario@edge.ufal.br) 
 * @version 0.1
 * @date 23/09/2026
 *
 * @copyright Copyright (c) Centro de Inovacao EDGE - 2026
 *
 *******************************************************************/

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "sensors/zbook_microphone.h"

LOG_MODULE_REGISTER(zbook_microphone_sample);

int main(void)
{
	int ret;
	uint16_t sample;
	uint16_t min;
	uint16_t max;
	uint32_t valid_samples;
	int64_t window_end;

	ret = zbook_microphone_init();
	if (ret) {
		LOG_ERR("zbook_microphone_init failed (%d)", ret);
		return ret;
	}

	while (1) {
		min = UINT16_MAX;
		max = 0;
		valid_samples = 0;
		window_end = k_uptime_get() + CONFIG_APP_SAMPLE_WINDOW_MS;

		while (k_uptime_get() < window_end) {
			ret = zbook_microphone_read(&sample);
			if (ret) {
				LOG_ERR("zbook_microphone_read failed (%d)", ret);
				continue;
			}

			valid_samples++;

			if (sample < min) {
				min = sample;
			}
			if (sample > max) {
				max = sample;
			}
		}

		if (valid_samples == 0) {
			LOG_WRN("mic: no valid samples in window");
		} else {
			LOG_INF("mic: min=%u max=%u peak-to-peak=%u", min, max, max - min);
		}

		k_sleep(K_MSEC(CONFIG_APP_REPORT_INTERVAL_MS));
	}

	return 0;
}
