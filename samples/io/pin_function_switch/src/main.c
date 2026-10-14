/*******************************************************************
 * @file main.c
 *
 * @brief Cycles one header pin through GPIO, PWM and ADC at runtime.
 *
 * @details GPIO45 can do all three functions. Every zbook_pin_set_function()
 * call releases whatever the pin did before and brings up the new function,
 * so the same pin is reused without a rebuild or a devicetree change.
 *
 * Wire an LED (through a resistor) or a scope probe to the pin to see the
 * GPIO blink and the PWM fade; the ADC phase prints the voltage it sees.
 *
 * @author João Matheus Nascimento Dias (joao.dias@edge.ufal.br)
 * @version 0.1
 * @date 13/10/2026
 *
 * @copyright Copyright (c) 2026
 *
 *******************************************************************/

#include "io/zbook_pin.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(zbook_pin_switch_sample, LOG_LEVEL_INF);

#define PIN ZBOOK_PIN_HEADER_IO_45

#define PWM_FREQ_HZ   1000
#define ADC_BITS      12
#define ADC_VREF_MV   3300
#define ADC_SAMPLES   5
#define STEP_DELAY_MS 250

static int phase_gpio(void)
{
	struct zbook_pin *pin;
	int ret = zbook_pin_set_function(PIN, &ZBOOK_PIN_CFG_GPIO_OUT(ZBOOK_PIN_HEADER_STATE_LOW),
					 &pin);

	if (ret != 0) {
		return ret;
	}

	LOG_INF("GPIO%d -> GPIO output: blinking", PIN);

	for (int i = 0; i < 6; i++) {
		zbook_pin_gpio_write(pin, i % 2 ? ZBOOK_PIN_HEADER_STATE_LOW
						: ZBOOK_PIN_HEADER_STATE_HIGH);
		k_sleep(K_MSEC(STEP_DELAY_MS));
	}

	return 0;
}

static int phase_pwm(void)
{
	struct zbook_pin *pin;
	int ret = zbook_pin_set_function(PIN, &ZBOOK_PIN_CFG_PWM(PWM_FREQ_HZ, 0), &pin);

	if (ret != 0) {
		return ret;
	}

	LOG_INF("GPIO%d -> PWM %d Hz: fading", PIN, PWM_FREQ_HZ);

	for (int duty = 0; duty <= 100; duty += 10) {
		zbook_pin_pwm_set_duty(pin, duty);
		k_sleep(K_MSEC(STEP_DELAY_MS / 2));
	}

	return 0;
}

static int phase_adc(void)
{
	struct zbook_pin *pin;
	int ret = zbook_pin_set_function(PIN, &ZBOOK_PIN_CFG_ADC(ADC_BITS), &pin);

	if (ret != 0) {
		return ret;
	}

	LOG_INF("GPIO%d -> ADC %d bits: sampling", PIN, ADC_BITS);

	for (int i = 0; i < ADC_SAMPLES; i++) {
		uint16_t raw;

		ret = zbook_pin_adc_read(pin, &raw);
		if (ret != 0) {
			return ret;
		}

		unsigned int mv = (raw * ADC_VREF_MV) / ((1U << ADC_BITS) - 1U);

		LOG_INF("raw=%u (~%u mV)", raw, mv);
		k_sleep(K_MSEC(STEP_DELAY_MS));
	}

	return 0;
}

int main(void)
{
	static int (*const phases[])(void) = {phase_gpio, phase_pwm, phase_adc};

	for (;;) {
		for (size_t i = 0; i < ARRAY_SIZE(phases); i++) {
			int ret = phases[i]();

			if (ret != 0) {
				LOG_ERR("phase %u failed (%d)", (unsigned int)i, ret);
			}
		}
	}

	return 0;
}
