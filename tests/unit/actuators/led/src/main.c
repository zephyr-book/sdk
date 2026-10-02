/************************************************************************
 * @file main.c
 *
 * @brief Unit tests for the Zbook LED actuator interface.
 * @author João Matheus Nascimento Dias (joao.dias@edge.ufal.br)
 *
 * @copyright Centro de Inovação EDGE 2026. Todos os direitos reservados.
 *
 ***********************************************************************/

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>

#include "actuators/zbook_led.h"

#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)
#define LED2_NODE DT_ALIAS(led2)
#define LED3_NODE DT_ALIAS(led3)

/* Index matches enum zbook_led (ZBOOK_LED_0..ZBOOK_LED_3). */
static const struct gpio_dt_spec test_led[] = {
	GPIO_DT_SPEC_GET(LED0_NODE, gpios),
	GPIO_DT_SPEC_GET(LED1_NODE, gpios),
	GPIO_DT_SPEC_GET(LED2_NODE, gpios),
	GPIO_DT_SPEC_GET(LED3_NODE, gpios),
};
	zassert_ok(zbook_led_off(ZBOOK_LED_ALL), "failed to reset LEDs off");
	zassert_ok(zbook_led_init(), "zbook_led_init failed");
}

ZTES

static int led_pin_value(enum zbook_led led)
{
	return gpio_emul_output_get(test_led[led].port, test_led[led].pin);
}

ZTEST_SUITE(zbook_hw_errors, NULL, NULL, NULL, NULL, NULL);

ZTEST(zbook_hw_errors, test_init_fails_while_device_not_ready)
{
	const struct device *gpio_ctrl = DEVICE_DT_GET(DT_NODELABEL(zbook_led_gpio));

	zassert_false(device_is_ready(gpio_ctrl), "test setup: gpio controller already ready");

	zassert_equal(-ENODEV, zbook_led_init());

	zassert_ok(device_init(gpio_ctrl), "failed to bring up gpio controller for later tests");
	zassert_true(device_is_ready(gpio_ctrl));
}

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
	zassert_ok(zbook_led_on(ZBOOK_LED_1));
	zassert_equal(1, led_pin_value(ZBOOK_LED_1));
}

ZTEST(zbook_led, test_off_sets_pin_low)
{
	zassert_ok(zbook_led_on(ZBOOK_LED_1));
	zassert_ok(zbook_led_off(ZBOOK_LED_1));
	zassert_equal(0, led_pin_value(ZBOOK_LED_1));
}

ZTEST(zbook_led, test_on_off_does_not_affect_other_leds)
{
	zassert_ok(zbook_led_on(ZBOOK_LED_0));

	zassert_equal(1, led_pin_value(ZBOOK_LED_0));
	zassert_equal(0, led_pin_value(ZBOOK_LED_1));
	zassert_equal(0, led_pin_value(ZBOOK_LED_2));
	zassert_equal(0, led_pin_value(ZBOOK_LED_3));
}

ZTEST(zbook_led, test_all_turns_on_every_led)
{
	zassert_ok(zbook_led_on(ZBOOK_LED_ALL));

	for (enum zbook_led led = ZBOOK_LED_0; led <= ZBOOK_LED_3; led++) {
		zassert_equal(1, led_pin_value(led), "led %d not on", led);
	}
}

ZTEST(zbook_led, test_all_turns_off_every_led)
{
	zassert_ok(zbook_led_on(ZBOOK_LED_ALL));
	zassert_ok(zbook_led_off(ZBOOK_LED_ALL));

	for (enum zbook_led led = ZBOOK_LED_0; led <= ZBOOK_LED_3; led++) {
		zassert_equal(0, led_pin_value(led), "led %d not off", led);
	}
}

ZTEST(zbook_led, test_on_rejects_invalid_led)
{
	zassert_equal(-EINVAL, zbook_led_on((enum zbook_led) - 1));
	zassert_equal(-EINVAL, zbook_led_on((enum zbook_led)(ZBOOK_LED_ALL + 1)));
}

ZTEST(zbook_led, test_off_rejects_invalid_led)
{
	zassert_equal(-EINVAL, zbook_led_off((enum zbook_led) - 1));
	zassert_equal(-EINVAL, zbook_led_off((enum zbook_led)(ZBOOK_LED_ALL + 1)));
}

ZTEST(zbook_led, test_blink_rejects_invalid_led)
{
	zassert_equal(-EINVAL, zbook_led_blink((enum zbook_led) - 1, ZBOOK_LED_BLINK_DEFAULT));
	zassert_equal(-EINVAL, zbook_led_blink((enum zbook_led)(ZBOOK_LED_ALL + 1),
					       ZBOOK_LED_BLINK_DEFAULT));
}

ZTEST(zbook_led, test_blink_rejects_invalid_state)
{
	zassert_equal(-EINVAL, zbook_led_blink(ZBOOK_LED_0, (enum zbook_led_state) - 1));
	zassert_equal(-EINVAL, zbook_led_blink(ZBOOK_LED_0, ZBOOK_LED_STATE_AMOUNT));
}

ZTEST(zbook_led, test_blink_fast_toggles_pin_periodically)
{
	zassert_ok(zbook_led_blink(ZBOOK_LED_2, ZBOOK_LED_BLINK_FAST));
	assert_led_blinks(ZBOOK_LED_2, CONFIG_ZBOOK_LED_BLINK_FAST_MS);
}

ZTEST(zbook_led, test_blink_default_toggles_pin_periodically)
{
	zassert_ok(zbook_led_blink(ZBOOK_LED_2, ZBOOK_LED_BLINK_DEFAULT));
	assert_led_blinks(ZBOOK_LED_2, CONFIG_ZBOOK_LED_BLINK_DEFAULT_MS);
}

ZTEST(zbook_led, test_blink_slow_toggles_pin_periodically)
{
	zassert_ok(zbook_led_blink(ZBOOK_LED_2, ZBOOK_LED_BLINK_SLOW));
	assert_led_blinks(ZBOOK_LED_2, CONFIG_ZBOOK_LED_BLINK_SLOW_MS);
}

ZTEST(zbook_led, test_blink_all_toggles_every_led)
{
	zassert_ok(zbook_led_blink(ZBOOK_LED_ALL, ZBOOK_LED_BLINK_FAST));

	for (enum zbook_led led = ZBOOK_LED_0; led <= ZBOOK_LED_3; led++) {
		assert_led_blinks(led, CONFIG_ZBOOK_LED_BLINK_FAST_MS);
	}
}

ZTEST(zbook_led, test_on_stops_blink)
{
	zassert_ok(zbook_led_blink(ZBOOK_LED_3, ZBOOK_LED_BLINK_FAST));
	k_msleep(CONFIG_ZBOOK_LED_BLINK_FAST_MS * 3);

	zassert_ok(zbook_led_on(ZBOOK_LED_3));

	for (int i = 0; i < 5; i++) {
		k_msleep(CONFIG_ZBOOK_LED_BLINK_FAST_MS);
		zassert_equal(1, led_pin_value(ZBOOK_LED_3),
			      "LED toggled after on() stopped the blink");
	}
}

ZTEST(zbook_led, test_off_stops_blink)
{
	zassert_ok(zbook_led_blink(ZBOOK_LED_3, ZBOOK_LED_BLINK_FAST));
	k_msleep(CONFIG_ZBOOK_LED_BLINK_FAST_MS * 3);

	zassert_ok(zbook_led_off(ZBOOK_LED_3));

	for (int i = 0; i < 5; i++) {
		k_msleep(CONFIG_ZBOOK_LED_BLINK_FAST_MS);
		zassert_equal(0, led_pin_value(ZBOOK_LED_3),
			      "LED toggled after off() stopped the blink");
	}
}

ZTEST(zbook_led, test_on_mask_affects_only_selected_leds)
{
	zassert_ok(zbook_led_on_mask(BIT(ZBOOK_LED_0) | BIT(ZBOOK_LED_2)));

	zassert_equal(1, led_pin_value(ZBOOK_LED_0));
	zassert_equal(0, led_pin_value(ZBOOK_LED_1));
	zassert_equal(1, led_pin_value(ZBOOK_LED_2));
	zassert_equal(0, led_pin_value(ZBOOK_LED_3));
}

ZTEST(zbook_led, test_off_mask_affects_only_selected_leds)
{
	zassert_ok(zbook_led_on_mask(BIT(ZBOOK_LED_ALL) - 1));
	zassert_ok(zbook_led_off_mask(BIT(ZBOOK_LED_1) | BIT(ZBOOK_LED_3)));

	zassert_equal(1, led_pin_value(ZBOOK_LED_0));
	zassert_equal(0, led_pin_value(ZBOOK_LED_1));
	zassert_equal(1, led_pin_value(ZBOOK_LED_2));
	zassert_equal(0, led_pin_value(ZBOOK_LED_3));
}

ZTEST(zbook_led, test_blink_mask_toggles_only_selected_leds)
{
	zassert_ok(zbook_led_blink_mask(BIT(ZBOOK_LED_1) | BIT(ZBOOK_LED_2), ZBOOK_LED_BLINK_FAST));

	assert_led_blinks(ZBOOK_LED_1, CONFIG_ZBOOK_LED_BLINK_FAST_MS);
	assert_led_blinks(ZBOOK_LED_2, CONFIG_ZBOOK_LED_BLINK_FAST_MS);
}

ZTEST(zbook_led, test_on_mask_rejects_invalid_bit)
{
	zassert_equal(-EINVAL, zbook_led_on_mask(BIT(ZBOOK_LED_ALL)));
}

ZTEST(zbook_led, test_toggle_flips_pin)
{
	zassert_ok(zbook_led_toggle(ZBOOK_LED_0));
	zassert_equal(1, led_pin_value(ZBOOK_LED_0));

	zassert_ok(zbook_led_toggle(ZBOOK_LED_0));
	zassert_equal(0, led_pin_value(ZBOOK_LED_0));
}

ZTEST(zbook_led, test_toggle_stops_blink)
{
	zassert_ok(zbook_led_blink(ZBOOK_LED_3, ZBOOK_LED_BLINK_FAST));
	k_msleep(CONFIG_ZBOOK_LED_BLINK_FAST_MS * 3);

	zassert_ok(zbook_led_toggle(ZBOOK_LED_3));
	int value_after_toggle = led_pin_value(ZBOOK_LED_3);

	for (int i = 0; i < 5; i++) {
		k_msleep(CONFIG_ZBOOK_LED_BLINK_FAST_MS);
		zassert_equal(value_after_toggle, led_pin_value(ZBOOK_LED_3),
			      "LED toggled again after toggle() stopped the blink");
	}
}

ZTEST(zbook_led, test_toggle_rejects_invalid_led)
{
	zassert_equal(-EINVAL, zbook_led_toggle((enum zbook_led) - 1));
	zassert_equal(-EINVAL, zbook_led_toggle((enum zbook_led)(ZBOOK_LED_ALL + 1)));
}

ZTEST(zbook_led, test_toggle_mask_affects_only_selected_leds)
{
	zassert_ok(zbook_led_toggle_mask(BIT(ZBOOK_LED_0) | BIT(ZBOOK_LED_2)));

	zassert_equal(1, led_pin_value(ZBOOK_LED_0));
	zassert_equal(0, led_pin_value(ZBOOK_LED_1));
	zassert_equal(1, led_pin_value(ZBOOK_LED_2));
	zassert_equal(0, led_pin_value(ZBOOK_LED_3));
}

ZTEST(zbook_led, test_toggle_mask_rejects_invalid_bit)
{
	zassert_equal(-EINVAL, zbook_led_toggle_mask(BIT(ZBOOK_LED_ALL)));
}

ZTEST(zbook_led, test_toggle_all_flips_every_led)
{
	zassert_ok(zbook_led_toggle(ZBOOK_LED_ALL));

	for (enum zbook_led led = ZBOOK_LED_0; led <= ZBOOK_LED_3; led++) {
		zassert_equal(1, led_pin_value(led), "led %d not on", led);
	}

	zassert_ok(zbook_led_toggle(ZBOOK_LED_ALL));

	for (enum zbook_led led = ZBOOK_LED_0; led <= ZBOOK_LED_3; led++) {
		zassert_equal(0, led_pin_value(led), "led %d not off", led);
	}
}
