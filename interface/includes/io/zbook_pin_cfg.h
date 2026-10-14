/*************************************************************************
 * @file zbook_pins.h
 *
 * @brief Defines the Interface for the Zbook header pins.
 * @author João Matheus Nascimento Dias (joao.dias@edge.ufal.br)
 * @version 0.1
 * @date 15/09/2026
 *
 * @copyright Centro de Inovação EDGE 2026. Todos os direitos reservados.
 *
 ***********************************************************************/
#ifndef ZBOOK_PINS_H
#define ZBOOK_PINS_H

#include <stdint.h>

/**
 * @brief GPIO numbers for the zbook header pins.
 *
 */
enum zbook_pin_header {
	ZBOOK_PIN_HEADER_IO_01 = 1,  /**< GPIO number for Zbook header pin 01 */
	ZBOOK_PIN_HEADER_IO_39 = 39, /**< GPIO number for Zbook header pin 39 */
	ZBOOK_PIN_HEADER_IO_45 = 45, /**< GPIO number for Zbook header pin 45 */
	ZBOOK_PIN_HEADER_IO_46 = 46, /**< GPIO number for Zbook header pin 46 */
	ZBOOK_PIN_HEADER_AMOUNT = 4, /**< Number of Zbook header pins */
	ZBOOK_PIN_HEADER_ALL = 0xFF, /**< All Zbook header pins */
};

/**
 * @brief Direction of the Zbook header pins.
 *
 */
enum zbook_pin_header_direction {
	ZBOOK_PIN_HEADER_DIRECTION_INPUT,  /**< Zbook header pin direction input */
	ZBOOK_PIN_HEADER_DIRECTION_OUTPUT, /**< Zbook header pin direction output */
};

/**
 * @brief Pull configuration of the Zbook header pins.
 *
 */
enum zbook_pin_header_pull {
	ZBOOK_PIN_HEADER_PULL_NONE, /**< Zbook header pin pull none */
	ZBOOK_PIN_HEADER_PULL_UP,   /**< Zbook header pin pull up */
	ZBOOK_PIN_HEADER_PULL_DOWN, /**< Zbook header pin pull down */
};

/**
 * @brief State of the Zbook header pins.
 *
 */
enum zbook_pin_header_state {
	ZBOOK_PIN_HEADER_STATE_LOW,        /**< Zbook header pin state low */
	ZBOOK_PIN_HEADER_STATE_HIGH,       /**< Zbook header pin state high */
	ZBOOK_PIN_HEADER_STATE_OPEN_DRAIN, /**< Zbook header pin state high impedance */
};

/**
 * @brief Interrupt configuration of the Zbook header pins.
 *
 */
enum zbook_pin_header_interrupt {
	ZBOOK_PIN_HEADER_INTERRUPT_NONE = 0,    /**< Zbook header pin interrupt none */
	ZBOOK_PIN_HEADER_INTERRUPT_RISING = 1,  /**< Zbook header pin interrupt rising edge */
	ZBOOK_PIN_HEADER_INTERRUPT_FALLING = 2, /**< Zbook header pin interrupt falling edge */
	ZBOOK_PIN_HEADER_INTERRUPT_BOTH = 3,    /**< Zbook header pin interrupt both edges */
};

/**
 * @brief Function of the Zbook header pins.
 *
 */
enum zbook_pin_header_function {
	ZBOOK_PIN_HEADER_FUNCTION_GPIO, /**< Zbook header pin function GPIO */
	ZBOOK_PIN_HEADER_FUNCTION_UART, /**< Zbook header pin function UART */
	ZBOOK_PIN_HEADER_FUNCTION_PWM,  /**< Zbook header pin function PWM */
	ZBOOK_PIN_HEADER_FUNCTION_ADC,  /**< Zbook header pin function ADC */
};

/**
 * @brief Direction a single header pin plays in a UART link.
 *
 */
enum zbook_pin_header_uart_role {
	ZBOOK_PIN_HEADER_UART_ROLE_TX, /**< Pin drives the TX line */
	ZBOOK_PIN_HEADER_UART_ROLE_RX, /**< Pin samples the RX line */
};

/**
 * @brief Polarity of the Zbook header pin PWM output.
 *
 */
enum zbook_pin_header_pwm_polarity {
	ZBOOK_PIN_HEADER_PWM_POLARITY_NORMAL,   /**< Duty cycle is time spent high. */
	ZBOOK_PIN_HEADER_PWM_POLARITY_INVERTED, /**< Duty cycle is time spent low. */
};

/**
 * @brief Configuration structure for the Zbook header pins.
 *
 */
struct zbook_pin_cfg_gpio {
	enum zbook_pin_header_direction direction; /**< Zbook header pin direction */
	enum zbook_pin_header_pull pull;           /**< Zbook header pin pull configuration */
	enum zbook_pin_header_state state;         /**< Zbook header pin state */
};

/**
 * @brief
 *
 */
struct zbook_pin_cfg_uart {
	enum zbook_pin_header_uart_role role; /**< Direction this pin drives (TX or RX) */
	uint32_t baudrate;                    /**< Zbook header pin UART baudrate */
	uint8_t data_bits; /**< Data bits per frame (5-9), 0 selects the default (8) */
	uint8_t parity;    /**< Parity mode, one of enum zbook_uart_parity (0 = none) */
};

/**
 * @brief Configuration structure for the Zbook header pin PWM.
 *
 */
struct zbook_pin_cfg_pwm {
	enum zbook_pin_header_pwm_polarity polarity; /**< Zbook header pin PWM polarity */
	uint32_t frequency;                          /**< Zbook header pin PWM frequency */
	uint8_t duty_cycle;                          /**< Zbook header pin PWM duty cycle */
};

/**
 * @brief Configuration structure for the Zbook header pin ADC.
 *
 */
struct zbook_pin_cfg_adc {
	uint8_t resolution;      /**< Zbook header pin ADC resolution */
	float reference_voltage; /**< Zbook header pin ADC reference voltage */
};

/**
 * @brief Configuration structure for the Zbook header pins.
 *
 */
struct zbook_pin_header_config {
	enum zbook_pin_header_function function; /**< Zbook header pin function */
	union {
		struct zbook_pin_cfg_gpio gpio; /**< Zbook header pin GPIO configuration */
		struct zbook_pin_cfg_uart uart; /**< Zbook header pin UART configuration */
		struct zbook_pin_cfg_pwm pwm;   /**< Zbook header pin PWM configuration */
		struct zbook_pin_cfg_adc adc;   /**< Zbook header pin ADC configuration */
	};
};

#endif /* ZBOOK_PINS_H */