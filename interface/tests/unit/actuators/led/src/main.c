/*******************************************************************
 * @file main.c
 *
 * @brief Unit tests for the Zbook LED actuator interface.
 * @author João Matheus Nascimento Dias (joao.dias@edge.ufal.br)
 *
 * @copyright Copyright (c) 2026
 *
 *******************************************************************/

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>

#include "actuators/zbook_led.h"

#define LED1_NODE DT_ALIAS(zbook_led0)
#define LED2_NODE DT_ALIAS(zbook_led1)
#define LED3_NODE DT_ALIAS(zbook_led2)
#define LED4_NODE DT_ALIAS(zbook_led3)

/* Index matches enum zbook_led (ZBOOK_LED_1..ZBOOK_LED_4). */
static const struct gpio_dt_spec test_led[] = {
	GPIO_DT_SPEC_GET(LED1_NODE, gpios),
	GPIO_DT_SPEC_GET(LED2_NODE, gpios),
	GPIO_DT_SPEC_GET(LED3_NODE, gpios),
	GPIO_DT_SPEC_GET(LED4_NODE, gpios),
};

static int led_pin_value(enum zbook_led led)
{
	return gpio_emul_output_get(test_led[led].port, test_led[led].pin);
}

/*
 * The gpio_emul controller backing led1..led4 is marked
 * "zephyr,deferred-init" in the overlay, so it boots "not ready". This suite
 * exercises the driver's is_devices_ready()/-ENODEV paths while that holds,
 * then brings the controller up (device_init()) for the rest of the run --
 * it must run before the "zbook_led" suite below, which assumes a ready
 * device in its fixture.
 */
ZTEST_SUITE(zbook_hw_errors, NULL, NULL, NULL, NULL, NULL);

ZTEST(zbook_hw_errors, test_calls_fail_while_device_not_ready)
{
	const struct device *gpio_ctrl = DEVICE_DT_GET(DT_NODELABEL(zbook_led_gpio));

	zassert_false(device_is_ready(gpio_ctrl), "test setup: gpio controller already ready");

	zassert_equal(-ENODEV, zbook_led_init());
	zassert_equal(-ENODEV, zbook_led_on(ZBOOK_LED_1));
	zassert_equal(-ENODEV, zbook_led_off(ZBOOK_LED_1));
	zassert_equal(-ENODEV, zbook_led_blink(ZBOOK_LED_1, ZBOOK_LED_BLINK_FAST));

	zassert_ok(device_init(gpio_ctrl), "failed to bring up gpio controller for later tests");
	zassert_true(device_is_ready(gpio_ctrl));
}

/* Sample much finer than any configured blink period so both edges are observed. */
static void assert_led_blinks(enum zbook_led led, uint32_t period_ms)
{
	bool saw_on = false;
	bool saw_off = false;

	for (int i = 0; i < period_ms * 4; i++) {
		k_msleep(1);

		if (led_pin_value(led)) {
			saw_on = true;
		} else {
			saw_off = true;
		}
	}

	zassert_true(saw_on, "led %d never observed on while blinking", led);
	zassert_true(saw_off, "led %d never observed off while blinking", led);
}

static void zbook_led_before(void *fixture)
{
	ARG_UNUSED(fixture);

	/*
	 * Stop any blink left running by the previous test *before*
	 * re-initializing the timers: zbook_led_init() re-inits the k_timer
	 * objects unconditionally, and re-initializing one still queued in
	 * the kernel's timeout list corrupts that list.
	 */
	zassert_ok(zbook_led_off(ZBOOK_LED_ALL), "failed to reset LEDs off");
	zassert_ok(zbook_led_init(), "zbook_led_init failed");
}

ZTEST_SUITE(zbook_led, NULL, NULL, zbook_led_before, NULL, NULL);

ZTEST(zbook_led, test_init_returns_ok_when_devices_ready)
{
	zassert_ok(zbook_led_init());
}

ZTEST(zbook_led, test_on_sets_pin_high)
{
	zassert_ok(zbook_led_on(ZBOOK_LED_2));
	zassert_equal(1, led_pin_value(ZBOOK_LED_2));
}

ZTEST(zbook_led, test_off_sets_pin_low)
{
	zassert_ok(zbook_led_on(ZBOOK_LED_2));
	zassert_ok(zbook_led_off(ZBOOK_LED_2));
	zassert_equal(0, led_pin_value(ZBOOK_LED_2));
}

ZTEST(zbook_led, test_on_off_does_not_affect_other_leds)
{
	zassert_ok(zbook_led_on(ZBOOK_LED_1));

	zassert_equal(1, led_pin_value(ZBOOK_LED_1));
	zassert_equal(0, led_pin_value(ZBOOK_LED_2));
	zassert_equal(0, led_pin_value(ZBOOK_LED_3));
	zassert_equal(0, led_pin_value(ZBOOK_LED_4));
}

ZTEST(zbook_led, test_all_turns_on_every_led)
{
	zassert_ok(zbook_led_on(ZBOOK_LED_ALL));

	for (enum zbook_led led = ZBOOK_LED_1; led <= ZBOOK_LED_4; led++) {
		zassert_equal(1, led_pin_value(led), "led %d not on", led);
	}
}

ZTEST(zbook_led, test_all_turns_off_every_led)
{
	zassert_ok(zbook_led_on(ZBOOK_LED_ALL));
	zassert_ok(zbook_led_off(ZBOOK_LED_ALL));

	for (enum zbook_led led = ZBOOK_LED_1; led <= ZBOOK_LED_4; led++) {
		zassert_equal(0, led_pin_value(led), "led %d not off", led);
	}
}

ZTEST(zbook_led, test_on_rejects_invalid_led)
{
	zassert_equal(-EINVAL, zbook_led_on((enum zbook_led)-1));
	zassert_equal(-EINVAL, zbook_led_on(_ZBOOK_LED_AMOUNT));
}

ZTEST(zbook_led, test_off_rejects_invalid_led)
{
	zassert_equal(-EINVAL, zbook_led_off((enum zbook_led)-1));
	zassert_equal(-EINVAL, zbook_led_off(_ZBOOK_LED_AMOUNT));
}

ZTEST(zbook_led, test_blink_rejects_invalid_led)
{
	zassert_equal(-EINVAL,
		      zbook_led_blink((enum zbook_led)-1, ZBOOK_LED_BLINK_DEFAULT));
	zassert_equal(-EINVAL, zbook_led_blink(_ZBOOK_LED_AMOUNT, ZBOOK_LED_BLINK_DEFAULT));
}

ZTEST(zbook_led, test_blink_rejects_invalid_state)
{
	zassert_equal(-EINVAL, zbook_led_blink(ZBOOK_LED_1, (enum zbook_led_state)-1));
	zassert_equal(-EINVAL, zbook_led_blink(ZBOOK_LED_1, _ZBOOK_LED_STATE_AMOUNT));
}

ZTEST(zbook_led, test_blink_fast_toggles_pin_periodically)
{
	zassert_ok(zbook_led_blink(ZBOOK_LED_3, ZBOOK_LED_BLINK_FAST));
	assert_led_blinks(ZBOOK_LED_3, CONFIG_ZBOOK_LED_BLINK_FAST_MS);
}

ZTEST(zbook_led, test_blink_default_toggles_pin_periodically)
{
	zassert_ok(zbook_led_blink(ZBOOK_LED_3, ZBOOK_LED_BLINK_DEFAULT));
	assert_led_blinks(ZBOOK_LED_3, CONFIG_ZBOOK_LED_BLINK_DEFAULT_MS);
}

ZTEST(zbook_led, test_blink_slow_toggles_pin_periodically)
{
	zassert_ok(zbook_led_blink(ZBOOK_LED_3, ZBOOK_LED_BLINK_SLOW));
	assert_led_blinks(ZBOOK_LED_3, CONFIG_ZBOOK_LED_BLINK_SLOW_MS);
}

ZTEST(zbook_led, test_blink_all_toggles_every_led)
{
	zassert_ok(zbook_led_blink(ZBOOK_LED_ALL, ZBOOK_LED_BLINK_FAST));

	for (enum zbook_led led = ZBOOK_LED_1; led <= ZBOOK_LED_4; led++) {
		assert_led_blinks(led, CONFIG_ZBOOK_LED_BLINK_FAST_MS);
	}
}

ZTEST(zbook_led, test_on_stops_blink)
{
	zassert_ok(zbook_led_blink(ZBOOK_LED_4, ZBOOK_LED_BLINK_FAST));
	k_msleep(CONFIG_ZBOOK_LED_BLINK_FAST_MS * 3);

	zassert_ok(zbook_led_on(ZBOOK_LED_4));

	for (int i = 0; i < 5; i++) {
		k_msleep(CONFIG_ZBOOK_LED_BLINK_FAST_MS);
		zassert_equal(1, led_pin_value(ZBOOK_LED_4),
			      "LED toggled after on() stopped the blink");
	}
}

ZTEST(zbook_led, test_off_stops_blink)
{
	zassert_ok(zbook_led_blink(ZBOOK_LED_4, ZBOOK_LED_BLINK_FAST));
	k_msleep(CONFIG_ZBOOK_LED_BLINK_FAST_MS * 3);

	zassert_ok(zbook_led_off(ZBOOK_LED_4));

	for (int i = 0; i < 5; i++) {
		k_msleep(CONFIG_ZBOOK_LED_BLINK_FAST_MS);
		zassert_equal(0, led_pin_value(ZBOOK_LED_4),
			      "LED toggled after off() stopped the blink");
	}
}
