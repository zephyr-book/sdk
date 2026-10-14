/*******************************************************************
 * @file main.c
 *
 * @brief Moves one header pin between functions at runtime.
 * @author João Matheus Nascimento Dias (joao.dias@edge.ufal.br)
 * @version 0.1
 * @date 08/10/2026
 *
 * @copyright Copyright (c) 2026
 *
 *******************************************************************/

#include "io/zbook_pin.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(zbook_pins_sample, LOG_LEVEL_INF);

#define PIN ZBOOK_PIN_HEADER_IO_01

int main(void)
{
	struct zbook_pin *pin;
	int ret;

	/* GPIO output: blink. */
	ret = zbook_pin_set_function(PIN, &ZBOOK_PIN_CFG_GPIO_OUT(ZBOOK_PIN_HEADER_STATE_LOW), &pin);
	if (ret != 0) {
		LOG_ERR("gpio out failed (%d)", ret);
		return 0;
	}

	for (int i = 0; i < 6; i++) {
		zbook_pin_gpio_write(pin, i % 2 ? ZBOOK_PIN_HEADER_STATE_LOW
						: ZBOOK_PIN_HEADER_STATE_HIGH);
		k_sleep(K_MSEC(250));
	}

	/* Same pin, now a GPIO input: the factory releases the output first. */
	ret = zbook_pin_set_function(PIN, &ZBOOK_PIN_CFG_GPIO_IN(ZBOOK_PIN_HEADER_PULL_UP), &pin);
	if (ret == 0) {
		enum zbook_pin_header_state state = ZBOOK_PIN_HEADER_STATE_LOW;

		if (zbook_pin_gpio_read(pin, &state) == 0) {
			LOG_INF("GPIO%d reads %s", PIN, state == ZBOOK_PIN_HEADER_STATE_HIGH ? "high" : "low");
		}
	}

	/* ADC is not wired on GPIO1: the factory refuses instead of misconfiguring. */
	ret = zbook_pin_set_function(PIN, &ZBOOK_PIN_CFG_ADC(12), NULL);
	LOG_INF("GPIO%d as ADC -> %d (expected -ENOTSUP)", PIN, ret);

	return 0;
}
