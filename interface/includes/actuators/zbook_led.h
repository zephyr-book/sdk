/*******************************************************************
 * @file zbook_led.h
 *
 * @brief Defines the Interface for the Zbook LED actuator.
 * @author João Matheus Nascimento Dias (joao.dias@edge.ufal.br)
 * @version 0.1
 * @date 11/09/2026
 *
 * @copyright Copyright (c) 2026
 *
 *******************************************************************/

#ifndef ZBOOK_LED_H
#define ZBOOK_LED_H

/**
 * @brief Defines the available LEDs on the Zbook device.
 *
 * @note: Referece at ZBook render to orientation.
 */
enum zbook_led {
	ZBOOK_LED_1 = 0,   /**< The led at left side*/
	ZBOOK_LED_2,       /**< The led at center left */
	ZBOOK_LED_3,       /**< The led at center right */
	ZBOOK_LED_4,       /**< The led at right side */
	ZBOOK_LED_ALL,     /**< All leds at the same action */
	_ZBOOK_LED_AMOUNT, /**< The amount of available LEDs */
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
	_ZBOOK_LED_STATE_AMOUNT, /**< The amount of available LED states */
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

#endif /* ZBOOK_LED_H */