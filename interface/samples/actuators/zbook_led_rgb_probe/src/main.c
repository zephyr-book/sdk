/*******************************************************************
 * @file main.c
 *
 * @brief Real-hardware bring-up sample for the zbook addressable RGB LED
 * actuator: cycles each LED individually through a set of test colors, then
 * all four together, then a set of colors built via
 * zbook_led_rgb_color_from_rgb24() (proving hex and decimal literals of the
 * same value produce the same color), then a mask pattern (even vs odd
 * LEDs). Meant to be watched on the real board -- a wrong color on one LED
 * usually means its color-mapping (channel order) is off, and a LED that
 * never lights at all (in the one-by-one or all-together phase) usually
 * means the led-strip devicetree node's chain-length doesn't match how many
 * LEDs are actually wired on the chain.
 *
 * @copyright Copyright (c) 2026
 *
 *******************************************************************/

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include "actuators/zbook_led_rgb.h"

#define STEP_DELAY  K_MSEC(400)
#define PHASE_DELAY K_MSEC(800)

static const struct {
	const char *name;
	struct zbook_led_rgb_color color;
} test_colors[] = {
	{"red", {.r = 0xC8}},
	{"green", {.g = 0xC8}},
	{"blue", {.b = 0xC8}},
	{"white", {.r = 0x20, .g = 0x20, .b = 0x20}},
	{"yellow", {.r = 0x40, .g = 0x40}},
	{"cyan", {.g = 0x40, .b = 0x40}},
	{"magenta", {.r = 0x40, .b = 0x40}},
	{"orange", {.r = 0xFF, .g = 0x8C}},
};

static void one_by_one(void)
{
	printk("-- one LED at a time --\n");

	for (enum zbook_led_rgb led = ZBOOK_LED_RGB_0; led < ZBOOK_LED_RGB_ALL; led++) {
		for (size_t c = 0; c < ARRAY_SIZE(test_colors); c++) {
			printk("  LED%d -> %s\n", led, test_colors[c].name);
			zbook_led_rgb_set(led, test_colors[c].color);
			k_sleep(STEP_DELAY);
		}
		zbook_led_rgb_off(led);
	}
}

static void all_together(void)
{
	printk("-- all LEDs together --\n");

	for (size_t c = 0; c < ARRAY_SIZE(test_colors); c++) {
		printk("  ALL -> %s\n", test_colors[c].name);
		zbook_led_rgb_set(ZBOOK_LED_RGB_ALL, test_colors[c].color);
		k_sleep(PHASE_DELAY);
	}

	zbook_led_rgb_off(ZBOOK_LED_RGB_ALL);
}

static void rgb24_colors(void)
{
	printk("-- zbook_led_rgb_color_from_rgb24(): hex vs decimal literals --\n");

	printk("  ALL -> tomato (0xFF6347)\n");
	zbook_led_rgb_set(ZBOOK_LED_RGB_ALL, zbook_led_rgb_color_from_rgb24(0xFF6347));
	k_sleep(PHASE_DELAY);

	printk("  ALL -> same color, written as decimal (2559971)\n");
	zbook_led_rgb_set(ZBOOK_LED_RGB_ALL, zbook_led_rgb_color_from_rgb24(2559971));
	k_sleep(PHASE_DELAY);

	printk("  ALL -> dodgerblue (0x1E90FF)\n");
	zbook_led_rgb_set(ZBOOK_LED_RGB_ALL, zbook_led_rgb_color_from_rgb24(0x1E90FF));
	k_sleep(PHASE_DELAY);

	printk("  ALL -> gold (0xFFD700)\n");
	zbook_led_rgb_set(ZBOOK_LED_RGB_ALL, zbook_led_rgb_color_from_rgb24(0xFFD700));
	k_sleep(PHASE_DELAY);

	zbook_led_rgb_off(ZBOOK_LED_RGB_ALL);
}

static void mask_pattern(void)
{
	printk("-- mask: even vs odd LEDs --\n");

	printk("  LED0+LED2 -> red, LED1+LED3 -> off\n");
	zbook_led_rgb_set_mask(BIT(ZBOOK_LED_RGB_0) | BIT(ZBOOK_LED_RGB_2), test_colors[0].color);
	zbook_led_rgb_off_mask(BIT(ZBOOK_LED_RGB_1) | BIT(ZBOOK_LED_RGB_3));
	k_sleep(PHASE_DELAY);

	printk("  LED1+LED3 -> blue, LED0+LED2 -> off\n");
	zbook_led_rgb_set_mask(BIT(ZBOOK_LED_RGB_1) | BIT(ZBOOK_LED_RGB_3), test_colors[2].color);
	zbook_led_rgb_off_mask(BIT(ZBOOK_LED_RGB_0) | BIT(ZBOOK_LED_RGB_2));
	k_sleep(PHASE_DELAY);

	zbook_led_rgb_off(ZBOOK_LED_RGB_ALL);
}

int main(void)
{
	int ret;

	printk("zbook RGB LED bring-up sample\n");

	ret = zbook_led_rgb_init();
	if (ret < 0) {
		printk("zbook_led_rgb_init failed: %d -- is the led-strip device ready?\n", ret);
		return 0;
	}

	while (1) {
		one_by_one();
		all_together();
		mask_pattern();
		rgb24_colors();
	}

	return 0;
}
