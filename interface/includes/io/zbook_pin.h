/*************************************************************************
 * @file zbook_pin.h
 *
 * @brief Runtime pin-function factory for the Zbook header pins: say
 * "pin X is function Y" and the pin is released from whatever it did
 * before and reconfigured for Y.
 * @author João Matheus Nascimento Dias (joao.dias@edge.ufal.br)
 * @version 0.1
 * @date 08/10/2026
 *
 * @copyright Centro de Inovação EDGE 2026. Todos os direitos reservados.
 *
 ***********************************************************************/
#ifndef ZBOOK_PIN_H
#define ZBOOK_PIN_H

#include "io/zbook_pin_cfg.h"
#include "io/zbook_io_adc.h"
#include "io/zbook_io_gpio.h"
#include "io/zbook_io_pwm.h"

#include <zephyr/kernel.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configuration structure for a Zbook header pin.
 *
 */
struct zbook_pin {
	struct k_mutex lock;                     /**< Mutex to protect the pin */
	enum zbook_pin_header gpio;              /**< GPIO number of the pin */
	bool configured;                         /**< Whether the pin is currently configured */
	enum zbook_pin_header_function function; /**< Current function of the pin */
	union {
		struct zbook_gpio_ctx gpio; /**< GPIO state, see zbook_io_gpio.h */

		// struct {
		// } uart;

		struct zbook_pwm_ctx pwm; /**< PWM state, see zbook_io_pwm.h */
		struct zbook_adc_ctx adc; /**< ADC state, see zbook_io_adc.h */
	} ctx;
};

/**
 * @brief Configures a header pin as a GPIO output.
 *
 * @param _state Initial state (@ref enum zbook_pin_header_state)
 */
#define ZBOOK_PIN_CFG_GPIO_OUT(_state)                                                             \
	((struct zbook_pin_header_config){                                                         \
		.function = ZBOOK_PIN_HEADER_FUNCTION_GPIO,                                        \
		.gpio = {.direction = ZBOOK_PIN_HEADER_DIRECTION_OUTPUT,                           \
			 .pull = ZBOOK_PIN_HEADER_PULL_NONE,                                       \
			 .state = (_state)},                                                       \
	})

/**
 * @brief Configures a header pin as a GPIO input.
 *
 * @param _pull Pull configuration for the GPIO pin (@ref enum zbook_pin_header_pull)
 */
#define ZBOOK_PIN_CFG_GPIO_IN(_pull)                                                               \
	((struct zbook_pin_header_config){                                                         \
		.function = ZBOOK_PIN_HEADER_FUNCTION_GPIO,                                        \
		.gpio = {.direction = ZBOOK_PIN_HEADER_DIRECTION_INPUT,                            \
			 .pull = (_pull),                                                          \
			 .state = ZBOOK_PIN_HEADER_STATE_LOW},                                     \
	})

/**
 * @brief Configures a header pin for UART transmission.
 *
 * @param _baud Baud rate for the UART communication.
 */
#define ZBOOK_PIN_CFG_UART_TX(_baud)                                                               \
	((struct zbook_pin_header_config){                                                         \
		.function = ZBOOK_PIN_HEADER_FUNCTION_UART,                                        \
		.uart = {.role = ZBOOK_PIN_HEADER_UART_ROLE_TX, .baudrate = (_baud)},              \
	})

/**
 * @brief Configures a header pin for UART reception.
 *
 * @param _baud Baud rate for the UART communication.
 */
#define ZBOOK_PIN_CFG_UART_RX(_baud)                                                               \
	((struct zbook_pin_header_config){                                                         \
		.function = ZBOOK_PIN_HEADER_FUNCTION_UART,                                        \
		.uart = {.role = ZBOOK_PIN_HEADER_UART_ROLE_RX, .baudrate = (_baud)},              \
	})

/**
 * @brief Configures a header pin for PWM output.
 *
 * @param _freq Frequency of the PWM signal in Hz.
 * @param _duty Duty cycle of the PWM signal in percentage (0-100).
 */
#define ZBOOK_PIN_CFG_PWM(_freq, _duty)                                                            \
	((struct zbook_pin_header_config){                                                         \
		.function = ZBOOK_PIN_HEADER_FUNCTION_PWM,                                         \
		.pwm = {.frequency = (_freq), .duty_cycle = (_duty)},                              \
	})

/**
 * @brief Configures a header pin for ADC input.
 *
 * @param _resolution Resolution of the ADC in bits.
 */
#define ZBOOK_PIN_CFG_ADC(_resolution)                                                             \
	((struct zbook_pin_header_config){                                                         \
		.function = ZBOOK_PIN_HEADER_FUNCTION_ADC,                                         \
		.adc = {.resolution = (_resolution)},                                              \
	})

/**
 * @brief Assigns a function to a header pin. If the pin already holds a
 * function, that one is torn down first (its resources released), then the
 * new one is brought up. Safe to call again with the same function to
 * reconfigure it.
 *
 * If bringing up the new function fails the pin is left unconfigured
 * (the old function is not restored).
 *
 * Not callable from interrupt context.
 *
 * @param pin[in] Header pin GPIO number (enum zbook_pin_header).
 * @param cfg[in] Function and its configuration.
 * @param handle[out] Optional. Set to the pin's handle on success.
 *
 * @retval 0 Success.
 * @retval -EINVAL pin is not a header pin, cfg is NULL, or cfg is invalid
 * for the function.
 * @retval -ENOTSUP The pin cannot do that function (e.g. ADC on a pin with
 * no ADC channel) or the function's backend is not built in.
 * @retval -EBUSY The function needs a hardware resource that is taken.
 * @retval other Backend-specific error.
 */
int zbook_pin_set_function(enum zbook_pin_header pin, const struct zbook_pin_header_config *cfg,
			   struct zbook_pin **handle);

/**
 * @brief Tears down the pin's current function and returns the pin to its
 * unconfigured state. Releasing an unconfigured pin is a no-op.
 *
 * @param pin[in] Pin handle.
 *
 * @retval 0 Success.
 * @retval -EINVAL handle is NULL or not a pin handle.
 */
int zbook_pin_release(struct zbook_pin *pin);

/**
 * @brief Whether a header pin is able to take a function (a static hardware
 * property; says nothing about resources currently in use).
 */
bool zbook_pin_supports(enum zbook_pin_header pin, enum zbook_pin_header_function function);

/**
 * @brief Current function of a header pin.
 *
 * @param pin[in] Header pin GPIO number.
 * @param function[out] Set to the pin's function when configured.
 *
 * @retval 0 Success.
 * @retval -EINVAL pin is not a header pin, or function is NULL.
 * @retval -ENODATA The pin is unconfigured.
 */
int zbook_pin_get_function(enum zbook_pin_header pin, enum zbook_pin_header_function *function);

/**
 * @brief Writes through the pin's current function. What the buffer means
 * depends on the function:
 * - GPIO: len == 1, buf[0] is an enum zbook_pin_header_state (output pins only).
 * - PWM: len == 1, buf[0] is the new duty cycle in percent (0-100).
 * - UART (TX role): len bytes are transmitted.
 *
 * @param handle[in] Pin handle.
 * @param buf[in] Data to write.
 * @param len[in] Number of bytes in buf.
 *
 * @retval 0 Success.
 * @retval -EINVAL handle/buf is NULL, the pin is unconfigured, or the
 * arguments are invalid for the function.
 * @retval -ENOTSUP The pin's function (or configured direction/role) cannot write.
 * @retval other Backend-specific error.
 */
int zbook_pin_write(struct zbook_pin *handle, const void *buf, size_t len);

/**
 * @brief Reads through the pin's current function. What comes back depends
 * on the function:
 * - GPIO: len == 1, buf[0] is set to ZBOOK_PIN_HEADER_STATE_LOW or _HIGH.
 * - ADC: len == sizeof(uint16_t), buf holds one sample at the configured resolution.
 * - UART (RX role): up to len received bytes (see zbook_uart_read()).
 *
 * @param handle[in] Pin handle.
 * @param buf[out] Buffer to fill.
 * @param len[in] Size of buf.
 *
 * @retval 0 Success.
 * @retval -EINVAL handle/buf is NULL, the pin is unconfigured, or the
 * arguments are invalid for the function.
 * @retval -ENOTSUP The pin's function (or configured direction/role) cannot read.
 * @retval other Backend-specific error.
 */
int zbook_pin_read(struct zbook_pin *handle, void *buf, size_t len);

/** @brief Typed helper: drives a GPIO output pin. See zbook_pin_write(). */
static inline int zbook_pin_gpio_write(struct zbook_pin *handle, enum zbook_pin_header_state state)
{
	uint8_t v = (uint8_t)state;

	return zbook_pin_write(handle, &v, sizeof(v));
}

/** @brief Typed helper: samples a GPIO pin. See zbook_pin_read(). */
static inline int zbook_pin_gpio_read(struct zbook_pin *handle, enum zbook_pin_header_state *state)
{
	uint8_t v;
	int ret = zbook_pin_read(handle, &v, sizeof(v));

	if (ret == 0 && state != NULL) {
		*state = (enum zbook_pin_header_state)v;
	}

	return ret;
}

/** @brief Typed helper: changes the duty cycle (0-100 %) of a PWM pin. See zbook_pin_write(). */
static inline int zbook_pin_pwm_set_duty(struct zbook_pin *handle, uint8_t duty_percent)
{
	return zbook_pin_write(handle, &duty_percent, sizeof(duty_percent));
}

/** @brief Typed helper: samples an ADC pin. See zbook_pin_read(). */
static inline int zbook_pin_adc_read(struct zbook_pin *handle, uint16_t *sample)
{
	return zbook_pin_read(handle, sample, sizeof(*sample));
}

#ifdef __cplusplus
}
#endif

#endif /* ZBOOK_PIN_H */
