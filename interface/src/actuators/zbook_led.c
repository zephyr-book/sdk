/*******************************************************************
 * @file zbook_led.c
 *
 * @brief Implements the interface for the Zbook LED actuator.
 * @author João Matheus Nascimento Dias (joao.dias@edge.ufal.br)
 * @version 0.1
 * @date 11/09/2026
 *
 * @copyright Copyright (c) 2026
 *
 *******************************************************************/

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "actuators/zbook_led.h"

LOG_MODULE_REGISTER(zbook_led, CONFIG_LED_LOG_LEVEL);

#define LED1_NODE DT_ALIAS(zbook_led0)
#define LED2_NODE DT_ALIAS(zbook_led1)
#define LED3_NODE DT_ALIAS(zbook_led2)
#define LED4_NODE DT_ALIAS(zbook_led3)

#define IS_LED_VALID(_led)                                                                         \
	do {                                                                                       \
		if (_led < 0 || _led >= _ZBOOK_LED_AMOUNT) {                                       \
			LOG_ERR("Invalid LED: %d", _led);                                          \
			return -EINVAL;                                                            \
		}                                                                                  \
	} while (0)

#define IS_STATE_VALID(_state)                                                                     \
	do {                                                                                       \
		if (_state < 0 || _state >= _ZBOOK_LED_STATE_AMOUNT) {                             \
			LOG_ERR("Invalid LED state: %d", _state);                                  \
			return -EINVAL;                                                            \
		}                                                                                  \
	} while (0)

struct {
	const struct gpio_dt_spec led[_ZBOOK_LED_AMOUNT - 1];
	enum zbook_led_state state[_ZBOOK_LED_AMOUNT - 1];
	struct k_timer blink_timer[_ZBOOK_LED_AMOUNT - 1];
} self = {
	.led =
		{
			GPIO_DT_SPEC_GET(LED1_NODE, gpios),
			GPIO_DT_SPEC_GET(LED2_NODE, gpios),
			GPIO_DT_SPEC_GET(LED3_NODE, gpios),
			GPIO_DT_SPEC_GET(LED4_NODE, gpios),
		},
};

typedef struct {
	int (*execute)(enum zbook_led led);
} led_fn_entry;

static void led_blink_expiry(struct k_timer *timer)
{
	intptr_t idx = (intptr_t)k_timer_user_data_get(timer);

	gpio_pin_toggle_dt(&self.led[idx]);
}

static int led_blink_stop(enum zbook_led led)
{
	if (led == ZBOOK_LED_ALL) {
		for (int i = 0; i < _ZBOOK_LED_AMOUNT - 1; i++) {
			k_timer_stop(&self.blink_timer[i]);
		}
		return 0;
	}

	k_timer_stop(&self.blink_timer[led]);
	return 0;
}

static int led_blink_start(enum zbook_led led, uint32_t period_ms)
{
	if (led == ZBOOK_LED_ALL) {
		for (int i = 0; i < _ZBOOK_LED_AMOUNT - 1; i++) {
			k_timer_start(&self.blink_timer[i], K_MSEC(period_ms), K_MSEC(period_ms));
		}
		return 0;
	}

	k_timer_start(&self.blink_timer[led], K_MSEC(period_ms), K_MSEC(period_ms));
	return 0;
}

static int led_off(enum zbook_led led)
{
	led_blink_stop(led);

	if (led == ZBOOK_LED_ALL) {
		for (int i = 0; i < _ZBOOK_LED_AMOUNT - 1; i++) {
			gpio_pin_set_dt(&self.led[i], 0);
		}
		return 0;
	}

	return gpio_pin_set_dt(&self.led[led], 0);
}

static int led_on(enum zbook_led led)
{
	led_blink_stop(led);

	if (led == ZBOOK_LED_ALL) {
		for (int i = 0; i < _ZBOOK_LED_AMOUNT - 1; i++) {
			gpio_pin_set_dt(&self.led[i], 1);
		}
		return 0;
	}

	return gpio_pin_set_dt(&self.led[led], 1);
}

static int led_blink_default(enum zbook_led led)
{
	return led_blink_start(led, CONFIG_ZBOOK_LED_BLINK_DEFAULT_MS);
}

static int led_blink_slow(enum zbook_led led)
{
	return led_blink_start(led, CONFIG_ZBOOK_LED_BLINK_SLOW_MS);
}

static int led_blink_fast(enum zbook_led led)
{
	return led_blink_start(led, CONFIG_ZBOOK_LED_BLINK_FAST_MS);
}

static const led_fn_entry led_fn_state[_ZBOOK_LED_STATE_AMOUNT] = {
	[ZBOOK_LED_OFF] = {.execute = led_off},
	[ZBOOK_LED_ON] = {.execute = led_on},
	[ZBOOK_LED_BLINK_DEFAULT] = {.execute = led_blink_default},
	[ZBOOK_LED_BLINK_SLOW] = {.execute = led_blink_slow},
	[ZBOOK_LED_BLINK_FAST] = {.execute = led_blink_fast},
};

static void set_state(enum zbook_led led, enum zbook_led_state state)
{
	if (led == ZBOOK_LED_ALL) {
		for (int i = 0; i < _ZBOOK_LED_AMOUNT - 1; i++) {
			self.state[i] = state;
		}
		return;
	}

	self.state[led] = state;
}

static inline int is_devices_ready(void)
{
	for (int i = 0; i < _ZBOOK_LED_AMOUNT - 1; i++) {
		if (!device_is_ready(self.led[i].port)) {
			return -ENODEV;
		}
	}

	return 0;
}

int zbook_led_init(void)
{
	int ret;

	ret = is_devices_ready();
	if (ret < 0) {
		LOG_ERR("LED device not ready (%s)", __func__);
		return ret;
	}

	for (int i = 0; i < _ZBOOK_LED_AMOUNT - 1; i++) {
		ret = gpio_pin_configure_dt(&self.led[i], GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_ERR("Failed to configure LED %d (%d)", i, ret);
			return ret;
		}

		/*
		 * Stop before (re-)initializing: re-running k_timer_init on a
		 * timer still queued in the kernel's timeout list corrupts it.
		 */
		k_timer_stop(&self.blink_timer[i]);
		k_timer_init(&self.blink_timer[i], led_blink_expiry, NULL);
		k_timer_user_data_set(&self.blink_timer[i], (void *)(intptr_t)i);
	}

	return 0;
}

int zbook_led_on(enum zbook_led led)
{
	LOG_DBG("Turning on LED %d", led);

	IS_LED_VALID(led);

	int ret;

	ret = is_devices_ready();
	if (ret < 0) {
		LOG_ERR("LED device not ready (%s)", __func__);
		return ret;
	}

	set_state(led, ZBOOK_LED_ON);
	return led_fn_state[ZBOOK_LED_ON].execute(led);
}

int zbook_led_off(enum zbook_led led)
{
	LOG_DBG("Turning off LED %d", led);

	IS_LED_VALID(led);

	int ret;

	ret = is_devices_ready();
	if (ret < 0) {
		LOG_ERR("LED device not ready (%s)", __func__);
		return ret;
	}

	set_state(led, ZBOOK_LED_OFF);
	return led_fn_state[ZBOOK_LED_OFF].execute(led);
}

int zbook_led_blink(enum zbook_led led, enum zbook_led_state state)
{
	LOG_DBG("Blinking LED %d with state %d - (%s)", led, state, __func__);

	IS_LED_VALID(led);
	IS_STATE_VALID(state);

	int ret;

	ret = is_devices_ready();
	if (ret < 0) {
		LOG_ERR("LED device not ready (%s)", __func__);
		return ret;
	}

	set_state(led, state);
	return led_fn_state[state].execute(led);
}
