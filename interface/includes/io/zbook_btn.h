/*************************************************************************
 * @file zbook_btn.h
 *
 * @brief Defines the Interface for the Zbook button input.
 * @author João Matheus Nascimento Dias (joao.dias@edge.ufal.br)
 * @version 0.2
 * @date 15/09/2026
 *
 * @copyright Centro de Inovação EDGE 2026. Todos os direitos reservados.
 *
 ***********************************************************************/

#ifndef ZBOOK_BTN_H
#define ZBOOK_BTN_H

#include <stdint.h>

#include <zephyr/sys/util.h>

/**
 * @brief Defines the available buttons on the Zbook device.
 *
 * @note: Referece at ZBook render to orientation.
 */
enum zbook_btn {
	ZBOOK_BTN_0 = 0, /**< The button at BTN0 */
	ZBOOK_BTN_1,     /**< The button at BTN1 */
	ZBOOK_BTN_2,     /**< The button at BTN2 */
	ZBOOK_BTN_3,     /**< The button at BTN3 */
	ZBOOK_BTN_ALL,   /**< All buttons at the same callback */
};

/**
 * @brief Defines the available events on the Zbook button input.
 *
 * @note ZBOOK_BTN_EVT_LONG_PRESSED fires once, after the button has been
 * held for CONFIG_BTN_LONG_PRESS_MS, in addition to (not instead of)
 * ZBOOK_BTN_EVT_PRESSED/ZBOOK_BTN_EVT_RELEASED.
 */
enum zbook_btn_evt {
	ZBOOK_BTN_EVT_NONE = 0,          /**< No event */
	ZBOOK_BTN_EVT_PRESSED = BIT(0),  /**< The event called on button press */
	ZBOOK_BTN_EVT_RELEASED = BIT(1), /**< The event called on button release */
	ZBOOK_BTN_EVT_LONG_PRESSED =
		BIT(2), /**< The event called once the button is held for the long-press delay */
	ZBOOK_BTN_EVT_BOTH =
		ZBOOK_BTN_EVT_PRESSED | ZBOOK_BTN_EVT_RELEASED, /**< Press and release events */
	ZBOOK_BTN_EVT_ALL = ZBOOK_BTN_EVT_PRESSED | ZBOOK_BTN_EVT_RELEASED |
			    ZBOOK_BTN_EVT_LONG_PRESSED, /**< Every available event */
};

/**
 * @brief Callback invoked on button event.
 *
 * @param btn[in] The button that triggered the event.
 * @param evt[in] The single event that fired (never a mask).
 * @param user_data[in] User-defined data passed to the callback.
 */
typedef void (*zbook_btn_cb_t)(enum zbook_btn btn, enum zbook_btn_evt evt, void *user_data);

/**
 * @brief Verify if the Zbook buttons is ready to use.
 *
 * @retval 0 Success: All Zbook buttons is ready to use.
 * @retval -ENODEV Error: The Zbook buttons input device is not ready.
 */
int zbook_btn_init(void);

/**
 * @brief Register a callback function for a specific button event.
 *
 * @param btn[in] The button for which to register the callback.
 * @param evt[in] Bitmask of events (see enum zbook_btn_evt) for which to invoke the callback.
 * @param cb[in] The callback function to register.
 * @param user_data[in] User-defined data to pass to the callback.
 *
 * @retval 0 Success: Callback registered successfully.
 * @retval -EINVAL Error: Invalid button or event mask specified, or callback is NULL.
 */
int zbook_btn_reg_cb(enum zbook_btn btn, enum zbook_btn_evt evt, zbook_btn_cb_t cb,
		     void *user_data);

/**
 * @brief Remove a callback function for a specific button event.
 *
 * @param btn[in] The button for which to remove the callback.
 *
 * @retval 0 Success: Callback removed successfully.
 * @retval -EINVAL Error: Invalid button specified.
 */
int zbook_btn_rm_cb(enum zbook_btn btn);

#endif /* ZBOOK_BTN_H */
