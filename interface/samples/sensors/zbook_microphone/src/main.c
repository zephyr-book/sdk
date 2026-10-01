/*******************************************************************
 * @file main.c
 *
 * @brief Sample: bring-up test for the ZBook microphone sensor. A single
 * raw sample doesn't say much about an AC audio signal on its own -- this
 * bursts as many reads as it can fit in a short window and reports the
 * min/max/peak-to-peak seen, once a second. Staying quiet should give a
 * small peak-to-peak (the mic sitting near its DC bias); making noise
 * (talking, clapping) near the mic should visibly widen it.
 *
 * @copyright Copyright (c) 2026
 *
 *******************************************************************/

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "sensors/zbook_microphone.h"

LOG_MODULE_REGISTER(zbook_microphone_sample);

#define SAMPLE_WINDOW_MS 200
#define REPORT_PERIOD     K_SECONDS(1)

int main(void)
{
	int ret;

	ret = zbook_microphone_init();
	if (ret) {
		LOG_ERR("zbook_microphone_init failed (%d)", ret);
		return ret;
	}

	while (1) {
		uint16_t min = UINT16_MAX;
		uint16_t max = 0;
		int64_t window_end = k_uptime_get() + SAMPLE_WINDOW_MS;

		while (k_uptime_get() < window_end) {
			uint16_t sample;

			ret = zbook_microphone_read(&sample);
			if (ret) {
				LOG_ERR("zbook_microphone_read failed (%d)", ret);
				continue;
			}

			if (sample < min) {
				min = sample;
			}
			if (sample > max) {
				max = sample;
			}
		}

		LOG_INF("mic: min=%u max=%u peak-to-peak=%u", min, max, max - min);

		k_sleep(REPORT_PERIOD);
	}

	return 0;
}
