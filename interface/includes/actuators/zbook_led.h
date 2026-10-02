/************************************************************************
 * @file zbook_led.h
 *
 * @brief Defines the Interface for the Zbook LED actuator.
 * @author João Matheus Nascimento Dias (joao.dias@edge.ufal.br)
 * @version 0.1
 * @date 11/09/2026
 *
 * @copyright Centro de Inovação EDGE 2026. Todos os direitos reservados.
 *
 ***********************************************************************/

#ifndef ZBOOK_LED_H
#define ZBOOK_LED_H

#include <stdint.h>

/**
 * @brief Defines the available LEDs on the Zbook device.
 *
 * @note: Referece at ZBook render to orientation.
 */
enum zbook_led {
	ZBOOK_LED_0 = 0, /**< The led at LED0 */
	ZBOOK_LED_1,     /**< The led at LED1 */
	ZBOOK_LED_2,     /**< The led at LED2 */
	ZBOOK_LED_3,     /**< The led at LED3 */
	ZBOOK_LED_ALL,   /**< All leds at the same action; also the LED count */
};

/**
 * @brief Defines the possible states for the Zbook LEDs.
 *
 * @note: The states are mutually exclusive.
 */
enum zbook_led_state {
	ZBOOK_LED_OFF = 0,       /**< Turn the led off */
	ZBOOK_LED_ON,            /**< Turn the led on */
	ZBOOK_LED_BLINK_DEFAULT, /**< Make the led blink with the default pattern */
	ZBOOK_LED_BLINK_SLOW,    /**< Make the led blink slowly */
	ZBOOK_LED_BLINK_FAST,    /**< Make the led blink quickly */
	ZBOOK_LED_STATE_AMOUNT,  /**< The amount of available LED states */
};

/**
 * @brief Verify if the Zbook LED actuator is ready to use.
 *
 * @retval 0 Success: All Zbook LED actuator is ready to use.
 * @retval -1 Error: At least one Zbook LED actuator is not ready to use.
 */
int zbook_led_init(void);

/**
 * @brief Turn on the specified Zbook LED.
 *
 * @param led[in] The LED to turn on.
 *
 * @retval 0 Success: The LED was turned on.
 * @retval -ENODEV Error: An error occurred while turning on the LED.
 * @retval -EINVAL Error: The specified led is invalid.
 */
int zbook_led_on(enum zbook_led led);

/**
 * @brief Turn off the specified Zbook LED.
 *
 * @param led[in] The LED to turn off.
 *
 * @retval 0 Success: The LED was turned off.
 * @retval -ENODEV Error: An error occurred while turning off the LED.
 * @retval -EINVAL Error: The specified led is invalid.
 */
int zbook_led_off(enum zbook_led led);

/**
 * @brief Make the specified Zbook LED blink with the given state.
 *
 * @param led[in] The LED to blink.
 * @param state[in] The blink state to set for the LED.
 *
 * @retval 0 Success: The LED was set to blink with the specified state.
 * @retval -ENODEV Error: An error occurred while setting the LED to blink.
 * @retval -EINVAL Error: The specified blink state or led is invalid.
 */
int zbook_led_blink(enum zbook_led led, enum zbook_led_state state);

/**
 * @brief Toggle the specified Zbook LED (on -> off, off -> on).
 *
 * Stops any ongoing blink on the LED first, same as zbook_led_on()/off().
 *
 * @param led[in] The LED to toggle.
 *
 * @retval 0 Success: The LED was toggled.
 * @retval -ENODEV Error: An error occurred while toggling the LED.
 * @retval -EINVAL Error: The specified led is invalid.
 */
int zbook_led_toggle(enum zbook_led led);

/**
 * @brief Turn on every Zbook LED set in the given bitmask.
 *
 * Convenience wrapper over zbook_led_on(): applies the action to each LED
 * whose bit (1 << ZBOOK_LED_n) is set, one at a time. Not atomic -- if one
 * LED fails, the ones already processed stay in their new state.
 *
 * @param led_mask[in] Bitmask of LEDs to turn on (e.g. BIT(ZBOOK_LED_0) | BIT(ZBOOK_LED_2)).
 *
 * @retval 0 Success: every requested LED was turned on.
 * @retval -ENODEV Error: an error occurred while turning on a LED.
 * @retval -EINVAL Error: the mask contains a bit outside the valid LED range.
 */
int zbook_led_on_mask(uint32_t led_mask);

/**
 * @brief Turn off every Zbook LED set in the given bitmask.
 *
 * Convenience wrapper over zbook_led_off(): applies the action to each LED
 * whose bit (1 << ZBOOK_LED_n) is set, one at a time. Not atomic -- if one
 * LED fails, the ones already processed stay in their new state.
 *
 * @param led_mask[in] Bitmask of LEDs to turn off (e.g. BIT(ZBOOK_LED_0) | BIT(ZBOOK_LED_2)).
 *
 * @retval 0 Success: every requested LED was turned off.
 * @retval -ENODEV Error: an error occurred while turning off a LED.
 * @retval -EINVAL Error: the mask contains a bit outside the valid LED range.
 */
int zbook_led_off_mask(uint32_t led_mask);

/**
 * @brief Make every Zbook LED set in the given bitmask blink with the given state.
 *
 * Convenience wrapper over zbook_led_blink(): applies the action to each LED
 * whose bit (1 << ZBOOK_LED_n) is set, one at a time. Not atomic -- if one
 * LED fails, the ones already processed stay in their new state.
 *
 * @param led_mask[in] Bitmask of LEDs to blink (e.g. BIT(ZBOOK_LED_0) | BIT(ZBOOK_LED_2)).
 * @param state[in] The blink state to set for the LEDs.
 *
 * @retval 0 Success: every requested LED was set to blink with the specified state.
 * @retval -ENODEV Error: an error occurred while setting a LED to blink.
 * @retval -EINVAL Error: the mask contains a bit outside the valid LED range, or the
 *         specified blink state is invalid.
 */
int zbook_led_blink_mask(uint32_t led_mask, enum zbook_led_state state);

/**
 * @brief Toggle every Zbook LED set in the given bitmask.
 *
 * Convenience wrapper over zbook_led_toggle(): applies the action to each LED
 * whose bit (1 << ZBOOK_LED_n) is set, one at a time. Not atomic -- if one
 * LED fails, the ones already processed stay toggled.
 *
 * @param led_mask[in] Bitmask of LEDs to toggle (e.g. BIT(ZBOOK_LED_0) | BIT(ZBOOK_LED_2)).
 *
 * @retval 0 Success: every requested LED was toggled.
 * @retval -ENODEV Error: an error occurred while toggling a LED.
 * @retval -EINVAL Error: the mask contains a bit outside the valid LED range.
 */
int zbook_led_toggle_mask(uint32_t led_mask);

#endif /* ZBOOK_LED_H */