/************************************************************************
 * @file zbook_led.c
 *
 * @brief Implements the interface for the Zbook LED actuator.
 * @author João Matheus Nascimento Dias (joao.dias@edge.ufal.br)
 * @version 0.1
 * @date 11/09/2026
 *
 * @copyright Centro de Inovação EDGE 2026. Todos os direitos reservados.
 *
 ***********************************************************************/

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "actuators/zbook_led.h"

LOG_MODULE_REGISTER(zbook_led, CONFIG_LED_LOG_LEVEL);

#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)
#define LED2_NODE DT_ALIAS(led2)
#define LED3_NODE DT_ALIAS(led3)

#define IS_LED_VALID(_led)                                                                         \
	do {                                                                                       \
		if (_led < 0 || _led > ZBOOK_LED_ALL) {                                            \
			LOG_ERR("Invalid LED: %d", _led);                                          \
			return -EINVAL;                                                            \
		}                                                                                  \
	} while (0)

#define IS_STATE_VALID(_state)                                                                     \
	do {                                                                                       \
		if (_state < 0 || _state >= ZBOOK_LED_STATE_AMOUNT) {                              \
			LOG_ERR("Invalid LED state: %d", _state);                                  \
			return -EINVAL;                                                            \
		}                                                                                  \
	} while (0)

static struct {
	const struct gpio_dt_spec led[ZBOOK_LED_ALL];
	enum zbook_led_state state[ZBOOK_LED_ALL];
	struct k_timer blink_timer;
	uint32_t blink_period_ms[ZBOOK_LED_ALL];
	int64_t blink_deadline[ZBOOK_LED_ALL];
	bool blinking[ZBOOK_LED_ALL];
} self = {
	.led =
		{
			GPIO_DT_SPEC_GET(LED0_NODE, gpios),
			GPIO_DT_SPEC_GET(LED1_NODE, gpios),
			GPIO_DT_SPEC_GET(LED2_NODE, gpios),
			GPIO_DT_SPEC_GET(LED3_NODE, gpios),
		},
};

typedef struct {
	int (*execute)(enum zbook_led led);
} led_fn_entry;

static void led_blink_rearm(void)
{
	int64_t now = k_uptime_get();
	int64_t next_deadline = INT64_MAX;

	for (int i = 0; i < ZBOOK_LED_ALL; i++) {
		if (self.blinking[i]) {
			next_deadline = MIN(next_deadline, self.blink_deadline[i]);
		}
	}

	if (next_deadline == INT64_MAX) {
		k_timer_stop(&self.blink_timer);
		return;
	}

	int64_t delay = MAX(next_deadline - now, 0);

	k_timer_start(&self.blink_timer, K_MSEC(delay), K_FOREVER);
}

static void led_blink_expiry(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	int64_t now = k_uptime_get();

	for (int i = 0; i < ZBOOK_LED_ALL; i++) {
		if (self.blinking[i] && self.blink_deadline[i] <= now) {
			gpio_port_toggle_bits(self.led[i].port, BIT(self.led[i].pin));
			self.blink_deadline[i] += self.blink_period_ms[i];
		}
	}

	led_blink_rearm();
}

static void led_blink_stop(enum zbook_led led)
{
	if (led == ZBOOK_LED_ALL) {
		for (int i = 0; i < ZBOOK_LED_ALL; i++) {
			self.blinking[i] = false;
		}
	} else {
		self.blinking[led] = false;
	}

	led_blink_rearm();
}

static void led_blink_start(enum zbook_led led, uint32_t period_ms)
{
	int64_t now = k_uptime_get();

	if (led == ZBOOK_LED_ALL) {
		for (int i = 0; i < ZBOOK_LED_ALL; i++) {
			self.blinking[i] = true;
			self.blink_period_ms[i] = period_ms;
			self.blink_deadline[i] = now + period_ms;
		}
	} else {
		self.blinking[led] = true;
		self.blink_period_ms[led] = period_ms;
		self.blink_deadline[led] = now + period_ms;
	}

	led_blink_rearm();
}

static int led_off(enum zbook_led led)
{
	led_blink_stop(led);

	if (led == ZBOOK_LED_ALL) {
		for (int i = 0; i < ZBOOK_LED_ALL; i++) {
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
		for (int i = 0; i < ZBOOK_LED_ALL; i++) {
			gpio_pin_set_dt(&self.led[i], 1);
		}
		return 0;
	}

	return gpio_pin_set_dt(&self.led[led], 1);
}

static int led_blink_default(enum zbook_led led)
{
	led_blink_start(led, CONFIG_ZBOOK_LED_BLINK_DEFAULT_MS);
	return 0;
}

static int led_blink_slow(enum zbook_led led)
{
	led_blink_start(led, CONFIG_ZBOOK_LED_BLINK_SLOW_MS);
	return 0;
}

static int led_blink_fast(enum zbook_led led)
{
	led_blink_start(led, CONFIG_ZBOOK_LED_BLINK_FAST_MS);
	return 0;
}

static const led_fn_entry led_fn_state[ZBOOK_LED_STATE_AMOUNT] = {
	[ZBOOK_LED_OFF] = {.execute = led_off},
	[ZBOOK_LED_ON] = {.execute = led_on},
	[ZBOOK_LED_BLINK_DEFAULT] = {.execute = led_blink_default},
	[ZBOOK_LED_BLINK_SLOW] = {.execute = led_blink_slow},
	[ZBOOK_LED_BLINK_FAST] = {.execute = led_blink_fast},
};

static void set_state(enum zbook_led led, enum zbook_led_state state)
{
	if (led == ZBOOK_LED_ALL) {
		for (int i = 0; i < ZBOOK_LED_ALL; i++) {
			self.state[i] = state;
		}
		return;
	}

	self.state[led] = state;
}

static inline int is_devices_ready(void)
{
	for (int i = 0; i < ZBOOK_LED_ALL; i++) {
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

	/*
	 * Stop before (re-)initializing: re-running k_timer_init on a timer
	 * still armed corrupts the kernel's timeout list.
	 */
	k_timer_stop(&self.blink_timer);
	k_timer_init(&self.blink_timer, led_blink_expiry, NULL);

	for (int i = 0; i < ZBOOK_LED_ALL; i++) {
		ret = gpio_pin_configure_dt(&self.led[i], GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_ERR("Failed to configure LED %d (%d)", i, ret);
			return ret;
		}

		self.blinking[i] = false;
	}

	return 0;
}

int zbook_led_on(enum zbook_led led)
{
	LOG_DBG("Turning on LED %d", led);

	IS_LED_VALID(led);

	set_state(led, ZBOOK_LED_ON);

	return led_fn_state[ZBOOK_LED_ON].execute(led);
}

int zbook_led_off(enum zbook_led led)
{
	LOG_DBG("Turning off LED %d", led);

	IS_LED_VALID(led);

	set_state(led, ZBOOK_LED_OFF);

	return led_fn_state[ZBOOK_LED_OFF].execute(led);
}

int zbook_led_blink(enum zbook_led led, enum zbook_led_state state)
{
	LOG_DBG("Blinking LED %d with state %d - (%s)", led, state, __func__);

	IS_LED_VALID(led);

	IS_STATE_VALID(state);

	set_state(led, state);

	return led_fn_state[state].execute(led);
}

static int led_toggle_one(int i)
{
	int ret = gpio_pin_toggle_dt(&self.led[i]);

	if (ret < 0) {
		return ret;
	}

	self.state[i] = gpio_pin_get_dt(&self.led[i]) ? ZBOOK_LED_ON : ZBOOK_LED_OFF;

	return 0;
}

int zbook_led_toggle(enum zbook_led led)
{
	LOG_DBG("Toggling LED %d", led);

	IS_LED_VALID(led);

	led_blink_stop(led);

	if (led == ZBOOK_LED_ALL) {
		for (int i = 0; i < ZBOOK_LED_ALL; i++) {
			int ret = led_toggle_one(i);

			if (ret < 0) {
				return ret;
			}
		}
		return 0;
	}

	return led_toggle_one(led);
}

static bool is_led_mask_valid(uint32_t led_mask)
{
	return led_mask != 0 && (led_mask & ~BIT_MASK(ZBOOK_LED_ALL)) == 0;
}

typedef int (*led_mask_action)(enum zbook_led led, void *arg);

/* Shared skeleton for the *_mask() wrappers: validate, then call fn() once
 * per set bit, stopping at the first failure. */
static int apply_to_mask(uint32_t led_mask, led_mask_action fn, void *arg)
{
	if (!is_led_mask_valid(led_mask)) {
		LOG_ERR("Invalid LED mask: 0x%x", led_mask);
		return -EINVAL;
	}

	for (int i = 0; i < ZBOOK_LED_ALL; i++) {
		if (led_mask & BIT(i)) {
			int ret = fn((enum zbook_led)i, arg);

			if (ret < 0) {
				return ret;
			}
		}
	}

	return 0;
}

static int on_action(enum zbook_led led, void *arg)
{
	ARG_UNUSED(arg);
	return zbook_led_on(led);
}

static int off_action(enum zbook_led led, void *arg)
{
	ARG_UNUSED(arg);
	return zbook_led_off(led);
}

static int blink_action(enum zbook_led led, void *arg)
{
	enum zbook_led_state state = *(enum zbook_led_state *)arg;

	return zbook_led_blink(led, state);
}

static int toggle_action(enum zbook_led led, void *arg)
{
	ARG_UNUSED(arg);
	return zbook_led_toggle(led);
}

int zbook_led_on_mask(uint32_t led_mask)
{
	return apply_to_mask(led_mask, on_action, NULL);
}

int zbook_led_off_mask(uint32_t led_mask)
{
	return apply_to_mask(led_mask, off_action, NULL);
}

int zbook_led_blink_mask(uint32_t led_mask, enum zbook_led_state state)
{
	return apply_to_mask(led_mask, blink_action, &state);
}

int zbook_led_toggle_mask(uint32_t led_mask)
{
	return apply_to_mask(led_mask, toggle_action, NULL);
}
