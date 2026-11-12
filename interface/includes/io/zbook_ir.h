/************************************************************************
 * @file zbook_ir.h
 *
 * @brief Defines the Interface for the Zbook infrared (IR) input/output.
 * @author Matheus Macário dos Santos (matheus.macario@edge.ufal.br)
 * @version 0.1
 * @date 10/11/2026
 *
 * @copyright Centro de Inovação EDGE 2026. Todos os direitos reservados.
 *
 ***********************************************************************/

#ifndef ZBOOK_IR_H
#define ZBOOK_IR_H

#include <stdint.h>

#include <zephyr/sys/util.h>

/**
 * @brief Defines the available events on the Zbook IR receiver.
 */
enum zbook_ir_evt {
	ZBOOK_IR_EVT_NONE,              /**< No event */
	ZBOOK_IR_EVT_DETECTED = BIT(0), /**< The receiver started detecting IR light */
	ZBOOK_IR_EVT_LOST = BIT(1),     /**< The receiver stopped detecting IR light */
	ZBOOK_IR_EVT_BOTH = ZBOOK_IR_EVT_DETECTED | ZBOOK_IR_EVT_LOST, /**< Every event */
};

/**
 * @brief Callback invoked on IR receiver event.
 *
 * @param[in] evt The single event that fired (never a mask).
 * @param[in] user_data User-defined data passed to the callback.
 */
typedef void (*zbook_ir_cb_t)(enum zbook_ir_evt evt, void *user_data);

/**
 * @brief Initialize the Zbook IR emitter and receiver.
 *
 * @retval 0 Success: The Zbook IR emitter and receiver are ready to use.
 * @retval -ENODEV Error: The GPIO device of the emitter or receiver is not ready.
 * @retval <0 Error: The GPIO driver failed to configure the emitter or receiver GPIO.
 */
int zbook_ir_init(void);

/**
 * @brief Turn on the Zbook IR emitter.
 *
 * @retval 0 Success: The IR emitter was turned on.
 * @retval -ENODEV Error: zbook_ir_init() has not succeeded yet.
 * @retval <0 Error: The GPIO driver failed to drive the emitter GPIO.
 */
int zbook_ir_emitter_on(void);

/**
 * @brief Turn off the Zbook IR emitter.
 *
 * @retval 0 Success: The IR emitter was turned off.
 * @retval -ENODEV Error: zbook_ir_init() has not succeeded yet.
 * @retval <0 Error: The GPIO driver failed to drive the emitter GPIO.
 */
int zbook_ir_emitter_off(void);

/**
 * @brief Toggle the Zbook IR emitter (on -> off, off -> on).
 *
 * @retval 0 Success: The IR emitter was toggled.
 * @retval -ENODEV Error: zbook_ir_init() has not succeeded yet.
 * @retval <0 Error: The GPIO driver failed to drive the emitter GPIO.
 */
int zbook_ir_emitter_toggle(void);

/**
 * @brief Get the current state of the Zbook IR emitter.
 *
 * @retval 1 The IR emitter is on.
 * @retval 0 The IR emitter is off.
 * @retval -ENODEV Error: zbook_ir_init() has not succeeded yet.
 */
int zbook_ir_emitter_get(void);

/**
 * @brief Turn on the Zbook IR emitter for the given duration, without blocking.
 *
 * @param[in] duration_ms How long the emitter stays on, in milliseconds, from 1
 * up to CONFIG_ZBOOK_INTERFACE_IO_IR_PULSE_MAX_MS.
 *
 * @retval 0 Success: The IR emitter was turned on and the pulse is running.
 * @retval -EINVAL Error: The duration is 0 or above CONFIG_ZBOOK_INTERFACE_IO_IR_PULSE_MAX_MS.
 * @retval -ENODEV Error: zbook_ir_init() has not succeeded yet.
 * @retval <0 Error: The GPIO driver failed to drive the emitter GPIO.
 */
int zbook_ir_emitter_pulse(uint32_t duration_ms);

/**
 * @brief Read the current level of the Zbook IR receiver.
 *
 * @retval 1 IR light is being detected.
 * @retval 0 No IR light is being detected.
 * @retval -ENODEV Error: zbook_ir_init() has not succeeded yet.
 * @retval <0 Error: The GPIO driver failed to read the receiver GPIO.
 */
int zbook_ir_receiver_read(void);

/**
 * @brief Register a callback function for the IR receiver events.
 *
 * @param[in] evt Bitmask of events (see enum zbook_ir_evt) for which to invoke the callback.
 * @param[in] cb The callback function to register.
 * @param[in] user_data User-defined data to pass to the callback.
 *
 * @retval 0 Success: Callback registered successfully.
 * @retval -EINVAL Error: Invalid event mask specified, or callback is NULL.
 * @retval -EWOULDBLOCK Error: Called from interrupt context.
 */
int zbook_ir_reg_cb(enum zbook_ir_evt evt, zbook_ir_cb_t cb, void *user_data);

/**
 * @brief Remove the callback function of the IR receiver events.
 *
 * @retval 0 Success: Callback removed successfully.
 * @retval -EWOULDBLOCK Error: Called from interrupt context.
 */
int zbook_ir_rm_cb(void);

#endif /* ZBOOK_IR_H */
