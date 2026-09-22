/*******************************************************************
 * @file zbook_led_rgb.h
 *
 * @brief Defines the interface for the Zbook addressable RGB LED actuator.
 * @author Matheus Macário dos Santos (matheus.macario@edge.ufal.br)
 * @version 0.1
 * @date 21/09/2026
 *
 * @copyright Copyright (c) 2026
 *
 *******************************************************************/

#ifndef ZBOOK_LED_RGB_H
#define ZBOOK_LED_RGB_H

#include <stdint.h>

/**
 * @brief Defines the available addressable RGB LEDs on the Zbook device.
 *
 * @note: Reference at ZBook render for orientation. Distinct from
 * enum zbook_led (interface/includes/actuators/zbook_led.h) -- these are
 * the WS2812-compatible chained LEDs (U7-U10 on the P2 schematic), driven
 * over a single-wire protocol, not the plain GPIO LED0-LED3.
 */
enum zbook_led_rgb {
	ZBOOK_LED_RGB_0 = 0, /**< The RGB LED at LED0 */
	ZBOOK_LED_RGB_1,     /**< The RGB LED at LED1 */
	ZBOOK_LED_RGB_2,     /**< The RGB LED at LED2 */
	ZBOOK_LED_RGB_3,     /**< The RGB LED at LED3 */
	ZBOOK_LED_RGB_ALL,   /**< All RGB LEDs at the same action; also the LED count */
};

/** @brief 24-bit color, one byte per channel. */
struct zbook_led_rgb_color {
	uint8_t r; /**< Red channel (0-255) */
	uint8_t g; /**< Green channel (0-255) */
	uint8_t b; /**< Blue channel (0-255) */
};

/**
 * @brief Build a zbook_led_rgb_color from a packed 24-bit RGB value.
 *
 * @p rgb24 is a plain uint32_t, so it accepts whatever numeral notation the
 * caller writes it in -- hex (0xFF6347) and decimal (16729927) are the same
 * value to the compiler, there is no separate "hex mode"/"decimal mode" to
 * pick. Only the low 24 bits are used; any bits above that are ignored.
 *
 * @param rgb24 Packed color as 0x00RRGGBB.
 * @return The equivalent struct zbook_led_rgb_color.
 */
struct zbook_led_rgb_color zbook_led_rgb_color_from_rgb24(uint32_t rgb24);

/**
 * @brief Verify if the Zbook RGB LED actuator is ready to use.
 *
 * @retval 0 Success: the Zbook RGB LED strip is ready to use.
 * @retval -ENODEV Error: the underlying led-strip device is not ready.
 */
int zbook_led_rgb_init(void);

/**
 * @brief Set the specified Zbook RGB LED to the given color.
 *
 * The WS2812 protocol has no per-pixel addressing -- every update sends the
 * state of the whole chain. This call updates the LED's entry in the
 * driver's local pixel buffer and immediately sends a fresh update for all
 * ZBOOK_LED_RGB_ALL pixels, so it never disturbs LEDs it wasn't asked to
 * change.
 *
 * @param led[in] The RGB LED to set.
 * @param color[in] The color to apply.
 *
 * @retval 0 Success: the LED was set to the given color.
 * @retval -ENODEV Error: an error occurred while updating the LED strip.
 * @retval -EINVAL Error: the specified led is invalid.
 */
int zbook_led_rgb_set(enum zbook_led_rgb led, struct zbook_led_rgb_color color);

/**
 * @brief Turn off the specified Zbook RGB LED (set it to black).
 *
 * @param led[in] The RGB LED to turn off.
 *
 * @retval 0 Success: the LED was turned off.
 * @retval -ENODEV Error: an error occurred while updating the LED strip.
 * @retval -EINVAL Error: the specified led is invalid.
 */
int zbook_led_rgb_off(enum zbook_led_rgb led);

/**
 * @brief Set every Zbook RGB LED set in the given bitmask to the given color.
 *
 * Unlike zbook_led_on_mask() and friends (interface/includes/actuators/
 * zbook_led.h), this is atomic: every requested pixel changes in the same
 * strip update, since the underlying protocol always sends the whole chain
 * in one frame anyway.
 *
 * @param led_mask[in] Bitmask of RGB LEDs to set (e.g. BIT(ZBOOK_LED_RGB_0) |
 *        BIT(ZBOOK_LED_RGB_2)).
 * @param color[in] The color to apply to every selected LED.
 *
 * @retval 0 Success: every requested LED was set to the given color.
 * @retval -ENODEV Error: an error occurred while updating the LED strip.
 * @retval -EINVAL Error: the mask contains a bit outside the valid LED range.
 */
int zbook_led_rgb_set_mask(uint32_t led_mask, struct zbook_led_rgb_color color);

/**
 * @brief Turn off every Zbook RGB LED set in the given bitmask.
 *
 * @param led_mask[in] Bitmask of RGB LEDs to turn off.
 *
 * @retval 0 Success: every requested LED was turned off.
 * @retval -ENODEV Error: an error occurred while updating the LED strip.
 * @retval -EINVAL Error: the mask contains a bit outside the valid LED range.
 */
int zbook_led_rgb_off_mask(uint32_t led_mask);

#endif /* ZBOOK_LED_RGB_H */
