/*
 * Fake led_strip driver backing the "led-strip" devicetree alias for unit
 * tests: captures whatever pixel buffer the last led_strip_update_rgb()
 * call sent, for a test to inspect afterward. No real WS2812/PIO peripheral
 * exists to emulate on native_sim, so this stands in for one.
 */

#include "zbook_led_rgb_fake_strip.h"

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/sys/util.h>

#define STRIP_NODE         DT_NODELABEL(zbook_led_rgb_fake)
#define MAX_CAPTURE_PIXELS 8

static struct led_rgb captured[MAX_CAPTURE_PIXELS];
static size_t captured_count;

void zbook_led_rgb_test_fake_reset(void)
{
	memset(captured, 0, sizeof(captured));
	captured_count = 0;
}

size_t zbook_led_rgb_test_fake_last_update(struct led_rgb *out, size_t max_len)
{
	size_t len = MIN(captured_count, max_len);

	memcpy(out, captured, len * sizeof(struct led_rgb));

	return len;
}

static int fake_strip_update_rgb(const struct device *dev, struct led_rgb *pixels,
				  size_t num_pixels)
{
	ARG_UNUSED(dev);

	captured_count = MIN(num_pixels, MAX_CAPTURE_PIXELS);
	memcpy(captured, pixels, captured_count * sizeof(struct led_rgb));

	return 0;
}

static int fake_strip_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static const struct led_strip_driver_api fake_strip_api = {
	.update_rgb = fake_strip_update_rgb,
};

DEVICE_DT_DEFINE(STRIP_NODE, fake_strip_init, NULL, NULL, NULL, POST_KERNEL,
		 CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &fake_strip_api);
