/*******************************************************************
 * @file main.c
 *
 * @brief Unit tests for the Zbook addressable RGB LED actuator interface.
 *
 * @copyright Copyright (c) 2026
 *
 *******************************************************************/

#include <errno.h>
#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/drivers/led_strip.h>

#include "actuators/zbook_led_rgb.h"
#include "zbook_led_rgb_fake_strip.h"

#define NUM_PIXELS ZBOOK_LED_RGB_ALL

#define BLACK ((struct zbook_led_rgb_color){0})
#define RED   ((struct zbook_led_rgb_color){.r = 0xFF})
#define GREEN ((struct zbook_led_rgb_color){.g = 0xFF})
#define BLUE  ((struct zbook_led_rgb_color){.b = 0xFF})

/* Every zbook_led_rgb_set()/off() call sends the whole strip in one frame,
 * so the fake's last capture always reflects the current state of every
 * pixel, not just the one a given call touched.
 */
static struct zbook_led_rgb_color pixel_color(enum zbook_led_rgb led)
{
	struct led_rgb pixels[NUM_PIXELS] = {0};

	zbook_led_rgb_test_fake_last_update(pixels, NUM_PIXELS);

	return (struct zbook_led_rgb_color){
		.r = pixels[led].r,
		.g = pixels[led].g,
		.b = pixels[led].b,
	};
}

static bool color_equal(struct zbook_led_rgb_color a, struct zbook_led_rgb_color b)
{
	return a.r == b.r && a.g == b.g && a.b == b.b;
}

/* Pure conversion, no hardware involved -- doesn't need the strip fixture. */
ZTEST_SUITE(zbook_led_rgb_color, NULL, NULL, NULL, NULL, NULL);

ZTEST(zbook_led_rgb_color, test_from_rgb24_splits_channels)
{
	/* "Tomato" (0xFF6347) written in hex vs. the exact same value written
	 * in decimal -- the function has no notion of "hex" vs "decimal", it
	 * just receives a uint32_t, so both calls must produce the same color.
	 */
	struct zbook_led_rgb_color hex = zbook_led_rgb_color_from_rgb24(0xFF6347);
	struct zbook_led_rgb_color decimal = zbook_led_rgb_color_from_rgb24(16737095);

	zassert_true(color_equal(hex, decimal));
	zassert_equal(0xFF, hex.r);
	zassert_equal(0x63, hex.g);
	zassert_equal(0x47, hex.b);
}

ZTEST(zbook_led_rgb_color, test_from_rgb24_black_and_white)
{
	zassert_true(color_equal(BLACK, zbook_led_rgb_color_from_rgb24(0x000000)));
	zassert_true(color_equal((struct zbook_led_rgb_color){0xFF, 0xFF, 0xFF},
				  zbook_led_rgb_color_from_rgb24(0xFFFFFF)));
}

ZTEST(zbook_led_rgb_color, test_from_rgb24_ignores_bits_above_24)
{
	struct zbook_led_rgb_color a = zbook_led_rgb_color_from_rgb24(0x00FF0000);
	struct zbook_led_rgb_color b = zbook_led_rgb_color_from_rgb24(0xAAFF0000);

	zassert_true(color_equal(a, b));
	zassert_true(color_equal(RED, a));
}

/*
 * The fake led-strip's devicetree node is marked "zephyr,deferred-init", so
 * it boots "not ready" -- this suite exercises zbook_led_rgb_init()'s
 * -ENODEV path while that holds, then brings the device up (device_init())
 * for the rest of the run. Must run before the "zbook_led_rgb" suite below,
 * which assumes a ready device in its fixture.
 */
ZTEST_SUITE(zbook_hw_errors, NULL, NULL, NULL, NULL, NULL);

ZTEST(zbook_hw_errors, test_init_fails_while_device_not_ready)
{
	const struct device *strip = DEVICE_DT_GET(DT_NODELABEL(zbook_led_rgb_fake));

	zassert_false(device_is_ready(strip), "test setup: strip already ready");

	zassert_equal(-ENODEV, zbook_led_rgb_init());

	zassert_ok(device_init(strip), "failed to bring up fake strip for later tests");
	zassert_true(device_is_ready(strip));
}

static void zbook_led_rgb_before(void *fixture)
{
	ARG_UNUSED(fixture);

	zbook_led_rgb_test_fake_reset();
	zassert_ok(zbook_led_rgb_init(), "zbook_led_rgb_init failed");
}

ZTEST_SUITE(zbook_led_rgb, NULL, NULL, zbook_led_rgb_before, NULL, NULL);

ZTEST(zbook_led_rgb, test_init_returns_ok_when_device_ready)
{
	zassert_ok(zbook_led_rgb_init());
}

ZTEST(zbook_led_rgb, test_init_clears_every_pixel)
{
	for (enum zbook_led_rgb led = ZBOOK_LED_RGB_0; led < ZBOOK_LED_RGB_ALL; led++) {
		zassert_true(color_equal(BLACK, pixel_color(led)), "led %d not cleared", led);
	}
}

ZTEST(zbook_led_rgb, test_set_updates_pixel_color)
{
	zassert_ok(zbook_led_rgb_set(ZBOOK_LED_RGB_1, RED));
	zassert_true(color_equal(RED, pixel_color(ZBOOK_LED_RGB_1)));
}

ZTEST(zbook_led_rgb, test_set_does_not_affect_other_pixels)
{
	zassert_ok(zbook_led_rgb_set(ZBOOK_LED_RGB_0, GREEN));

	zassert_true(color_equal(GREEN, pixel_color(ZBOOK_LED_RGB_0)));
	zassert_true(color_equal(BLACK, pixel_color(ZBOOK_LED_RGB_1)));
	zassert_true(color_equal(BLACK, pixel_color(ZBOOK_LED_RGB_2)));
	zassert_true(color_equal(BLACK, pixel_color(ZBOOK_LED_RGB_3)));
}

ZTEST(zbook_led_rgb, test_off_sets_pixel_black)
{
	zassert_ok(zbook_led_rgb_set(ZBOOK_LED_RGB_1, BLUE));
	zassert_ok(zbook_led_rgb_off(ZBOOK_LED_RGB_1));
	zassert_true(color_equal(BLACK, pixel_color(ZBOOK_LED_RGB_1)));
}

ZTEST(zbook_led_rgb, test_all_sets_every_pixel)
{
	zassert_ok(zbook_led_rgb_set(ZBOOK_LED_RGB_ALL, RED));

	for (enum zbook_led_rgb led = ZBOOK_LED_RGB_0; led < ZBOOK_LED_RGB_ALL; led++) {
		zassert_true(color_equal(RED, pixel_color(led)), "led %d not set", led);
	}
}

ZTEST(zbook_led_rgb, test_off_all_turns_off_every_pixel)
{
	zassert_ok(zbook_led_rgb_set(ZBOOK_LED_RGB_ALL, RED));
	zassert_ok(zbook_led_rgb_off(ZBOOK_LED_RGB_ALL));

	for (enum zbook_led_rgb led = ZBOOK_LED_RGB_0; led < ZBOOK_LED_RGB_ALL; led++) {
		zassert_true(color_equal(BLACK, pixel_color(led)), "led %d not off", led);
	}
}

ZTEST(zbook_led_rgb, test_set_rejects_invalid_led)
{
	zassert_equal(-EINVAL, zbook_led_rgb_set((enum zbook_led_rgb)-1, RED));
	zassert_equal(-EINVAL, zbook_led_rgb_set((enum zbook_led_rgb)(ZBOOK_LED_RGB_ALL + 1), RED));
}

ZTEST(zbook_led_rgb, test_off_rejects_invalid_led)
{
	zassert_equal(-EINVAL, zbook_led_rgb_off((enum zbook_led_rgb)-1));
	zassert_equal(-EINVAL, zbook_led_rgb_off((enum zbook_led_rgb)(ZBOOK_LED_RGB_ALL + 1)));
}

ZTEST(zbook_led_rgb, test_set_mask_affects_only_selected_pixels)
{
	zassert_ok(zbook_led_rgb_set_mask(BIT(ZBOOK_LED_RGB_0) | BIT(ZBOOK_LED_RGB_2), GREEN));

	zassert_true(color_equal(GREEN, pixel_color(ZBOOK_LED_RGB_0)));
	zassert_true(color_equal(BLACK, pixel_color(ZBOOK_LED_RGB_1)));
	zassert_true(color_equal(GREEN, pixel_color(ZBOOK_LED_RGB_2)));
	zassert_true(color_equal(BLACK, pixel_color(ZBOOK_LED_RGB_3)));
}

ZTEST(zbook_led_rgb, test_off_mask_affects_only_selected_pixels)
{
	zassert_ok(zbook_led_rgb_set_mask(BIT(ZBOOK_LED_RGB_ALL) - 1, RED));
	zassert_ok(zbook_led_rgb_off_mask(BIT(ZBOOK_LED_RGB_1) | BIT(ZBOOK_LED_RGB_3)));

	zassert_true(color_equal(RED, pixel_color(ZBOOK_LED_RGB_0)));
	zassert_true(color_equal(BLACK, pixel_color(ZBOOK_LED_RGB_1)));
	zassert_true(color_equal(RED, pixel_color(ZBOOK_LED_RGB_2)));
	zassert_true(color_equal(BLACK, pixel_color(ZBOOK_LED_RGB_3)));
}

ZTEST(zbook_led_rgb, test_set_mask_rejects_invalid_bit)
{
	zassert_equal(-EINVAL, zbook_led_rgb_set_mask(BIT(ZBOOK_LED_RGB_ALL), RED));
	zassert_equal(-EINVAL, zbook_led_rgb_set_mask(0, RED));
}

ZTEST(zbook_led_rgb, test_off_mask_rejects_invalid_bit)
{
	zassert_equal(-EINVAL, zbook_led_rgb_off_mask(BIT(ZBOOK_LED_RGB_ALL)));
	zassert_equal(-EINVAL, zbook_led_rgb_off_mask(0));
}
