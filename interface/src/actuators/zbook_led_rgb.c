/*******************************************************************
 * @file zbook_led_rgb.c
 *
 * @brief Implements the interface for the Zbook addressable RGB LED actuator.
 * @author Matheus Macário dos Santos (matheus.macario@edge.ufal.br)
 * @version 0.1
 * @date 21/09/2026
 *
 * @copyright Copyright (c) 2026
 *
 *******************************************************************/

#include <errno.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/logging/log.h>

#include "actuators/zbook_led_rgb.h"

LOG_MODULE_REGISTER(zbook_led_rgb, CONFIG_LED_RGB_LOG_LEVEL);

#define LED_STRIP_NODE DT_ALIAS(led_strip)

#define IS_LED_VALID(_led)                                                                        \
	do {                                                                                       \
		if (_led < 0 || _led > ZBOOK_LED_RGB_ALL) {                                       \
			LOG_ERR("Invalid RGB LED: %d", _led);                                     \
			return -EINVAL;                                                           \
		}                                                                                  \
	} while (0)

static const struct device *const strip = DEVICE_DT_GET(LED_STRIP_NODE);
static struct led_rgb pixels[ZBOOK_LED_RGB_ALL];

static int strip_update(void)
{
	int ret = led_strip_update_rgb(strip, pixels, ZBOOK_LED_RGB_ALL);

	if (ret < 0) {
		LOG_ERR("Failed to update RGB LED strip (%d)", ret);
	}

	return ret;
}

static void set_pixel(int i, struct zbook_led_rgb_color color)
{
	pixels[i] = (struct led_rgb){.r = color.r, .g = color.g, .b = color.b};
}

struct zbook_led_rgb_color zbook_led_rgb_color_from_rgb24(uint32_t rgb24)
{
	return (struct zbook_led_rgb_color){
		.r = (rgb24 >> 16) & 0xFF,
		.g = (rgb24 >> 8) & 0xFF,
		.b = rgb24 & 0xFF,
	};
}

int zbook_led_rgb_init(void)
{
	if (!device_is_ready(strip)) {
		LOG_ERR("RGB LED strip device not ready");
		return -ENODEV;
	}

	memset(pixels, 0, sizeof(pixels));

	return strip_update();
}

int zbook_led_rgb_set(enum zbook_led_rgb led, struct zbook_led_rgb_color color)
{
	LOG_DBG("Setting RGB LED %d to r=%u g=%u b=%u", led, color.r, color.g, color.b);

	IS_LED_VALID(led);

	if (led == ZBOOK_LED_RGB_ALL) {
		for (int i = 0; i < ZBOOK_LED_RGB_ALL; i++) {
			set_pixel(i, color);
		}
	} else {
		set_pixel(led, color);
	}

	return strip_update();
}

int zbook_led_rgb_off(enum zbook_led_rgb led)
{
	LOG_DBG("Turning off RGB LED %d", led);

	return zbook_led_rgb_set(led, (struct zbook_led_rgb_color){0});
}

static bool is_led_mask_valid(uint32_t led_mask)
{
	return led_mask != 0 && (led_mask & ~BIT_MASK(ZBOOK_LED_RGB_ALL)) == 0;
}

int zbook_led_rgb_set_mask(uint32_t led_mask, struct zbook_led_rgb_color color)
{
	LOG_DBG("Setting RGB LED mask 0x%x to r=%u g=%u b=%u", led_mask, color.r, color.g,
		color.b);

	if (!is_led_mask_valid(led_mask)) {
		LOG_ERR("Invalid RGB LED mask: 0x%x", led_mask);
		return -EINVAL;
	}

	for (int i = 0; i < ZBOOK_LED_RGB_ALL; i++) {
		if (led_mask & BIT(i)) {
			set_pixel(i, color);
		}
	}

	return strip_update();
}

int zbook_led_rgb_off_mask(uint32_t led_mask)
{
	LOG_DBG("Turning off RGB LED mask 0x%x", led_mask);

	return zbook_led_rgb_set_mask(led_mask, (struct zbook_led_rgb_color){0});
}
