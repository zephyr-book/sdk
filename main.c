/*******************************************************************
 * @file main.c
 *
 * @brief Zbook LED actuator demo.
 * @author João Matheus Nascimento Dias (joao.dias@edge.ufal.br)
 *
 * @copyright Copyright (c) 2026
 *
 *******************************************************************/

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "actuators/zbook_led.h"

LOG_MODULE_REGISTER(main);

int main(void)
{
	int ret;

	ret = zbook_led_init();
	if (ret < 0) {
		LOG_ERR("zbook_led_init failed (%d)", ret);
		return ret;
	}

	zbook_led_blink(ZBOOK_LED_ALL, ZBOOK_LED_BLINK_DEFAULT);

	return 0;
}
