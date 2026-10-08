/*******************************************************************
 * @file zbook_uart.h
 *
 * @brief Defines the Interface for the Zbook UART protocol.
 * @author João Matheus Nascimento Dias (joao.dias@edge.ufal.br)
 * @version 0.5
 * @date 25/09/2026
 *
 * @copyright Copyright (c) 2026
 *
 *******************************************************************/

#ifndef ZBOOK_UART_H
#define ZBOOK_UART_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/devicetree.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief GPIO numbers for the zbook UART protocol.
 */
enum zbook_uart_gpio {
	ZBOOK_UART_GPIO_01 = 1,  /**> GPIO 01 */
	ZBOOK_UART_GPIO_39 = 39, /**> GPIO 39 */
	ZBOOK_UART_GPIO_45 = 45, /**> GPIO 45 */
	ZBOOK_UART_GPIO_46 = 46, /**> GPIO 46 */
};

/**
 * @brief Parity mode for a zbook UART channel.
 */
enum zbook_uart_parity {
	ZBOOK_UART_PARITY_NONE = 0,
	ZBOOK_UART_PARITY_ODD,
	ZBOOK_UART_PARITY_EVEN,
};

/**
 * @brief Sentinel for zbook_uart_config's tx_pin/rx_pin (or zbook_uart_init()'s
 * tx_pin/rx_pin arguments): configure that direction as unused instead of
 * claiming a real GPIO/PIO state machine for it. Useful for a TX-only or
 * RX-only channel (e.g. two separate boards each only driving one
 * direction of a point-to-point link) -- setting both to this value is
 * rejected (nothing to configure).
 */
#define ZBOOK_UART_PIN_NONE UINT32_MAX

/**
 * @brief Line configuration for zbook_uart_configure().
 */
struct zbook_uart_config {
	uint32_t tx_pin;   /**! GPIO number to drive as TX, or ZBOOK_UART_PIN_NONE. */
	uint32_t rx_pin;   /**! GPIO number to sample as RX, or ZBOOK_UART_PIN_NONE. */
	uint32_t baudrate; /**! Baud rate. */
	uint8_t data_bits; /**! Data bits per frame, 5-9. */
	uint8_t parity;    /**! One of enum zbook_uart_parity. */
};

/**
 * @brief 
 * 
 */
struct zbook_uart_channel;

/**
 * @brief Callback type for zbook_uart_set_rx_callback().
 *
 * @param handle[in] Channel on which the byte was received.
 * @param byte[in] The received byte.
 * @param user_data[in] Opaque pointer passed through from zbook_uart_set_rx_callback().
 */
typedef void (*zbook_uart_rx_cb_t)(struct zbook_uart_channel *handle, uint8_t byte,
				   void *user_data);

/*
 * struct zbook_uart_channel: layout differs per target because the two
 * backends track completely different state: direct PIO hardware
 * (tx_pio/rx_pio/tx_sm/rx_sm/...) vs. the compatible node raspberrypi_pico_pio
 * if used as a stand-in (dev/tx_pin/rx_pin).
 */
#if DT_HAS_COMPAT_STATUS_OKAY(raspberrypi_pico_pio)

#include <hardware/pio.h>

/**
 * @brief Represents a zbook UART channel, which is a pair of GPIOs bit-banged
 *
 */
struct zbook_uart_channel {
	PIO tx_pio;               /**< The PIO block used for TX. */
	PIO rx_pio;               /**< The PIO block used for RX. */
	uint32_t tx_sm;           /**< The state machine used for TX. */
	uint32_t rx_sm;           /**< The state machine used for RX. */
	uint32_t tx_offset;       /**< The offset of the TX program in the PIO memory. */
	uint32_t rx_offset;       /**< The offset of the RX program in the PIO memory. */
	uint8_t data_bits;        /**< The number of data bits per frame. */
	uint8_t parity;           /**< The parity mode. */
	bool in_use;              /**< Whether the channel is in use. */
	void *rx_user_data;       /**< The user data to be passed to the RX callback. */
	zbook_uart_rx_cb_t rx_cb; /**< The callback function to rx callback. */
};

#else

#include <zephyr/device.h>

/**
 * @brief Represents a zbook UART channel, which is a pair of GPIOs bit-banged
 *
 */
struct zbook_uart_channel {
	const struct device *dev; /**< The Zephyr device for the UART. */
	uint32_t tx_pin;          /**< The GPIO number to drive as TX. */
	uint32_t rx_pin;          /**< The GPIO number to sample as RX. */
};

#endif /* DT_HAS_COMPAT_STATUS_OKAY(raspberrypi_pico_pio) */

/**
 * @brief Opens a zbook UART channel bit-banged over the given TX/RX GPIO
 * pair, with the given line configuration (data bits, parity).
 *
 * @param config[in] Line configuration. data_bits must be 5-9.
 * @param handle[out] Set to the opened channel on success.
 *
 * @retval 0 Success.
 * @retval -EINVAL Error: config is NULL, handle is NULL, data_bits is out
 * of range, or both tx_pin and rx_pin are ZBOOK_UART_PIN_NONE.
 * @retval -EBUSY Error: no free PIO state machines/program slots available.
 * @retval -ENODEV Error: no PIO block can address tx_pin or rx_pin.
 */
int zbook_uart_configure(const struct zbook_uart_config *config,
			 struct zbook_uart_channel **handle);

/**
 * @brief Opens a zbook UART channel bit-banged (8N1, the common case) over
 * the given TX/RX GPIO pair. Convenience wrapper around
 * zbook_uart_configure().
 *
 * @param tx_pin[in] GPIO number to drive as TX.
 * @param rx_pin[in] GPIO number to sample as RX.
 * @param baudrate[in] Baud rate.
 * @param handle[out] Set to the opened channel on success.
 *
 * @see zbook_uart_configure().
 */
int zbook_uart_init(uint32_t tx_pin, uint32_t rx_pin, uint32_t baudrate,
		    struct zbook_uart_channel **handle);

/**
 * @brief Closes a channel opened with zbook_uart_init(), freeing its state
 * machines and PIO program slots for reuse.
 *
 * @param handle[in] Channel to close.
 *
 * @retval 0 Success.
 * @retval -EINVAL handle is NULL or was not open.
 */
int zbook_uart_close(struct zbook_uart_channel *handle);

/**
 * @brief Writes data to an open zbook UART channel.
 *
 * @param handle[in] Channel to write to.
 * @param data[in] Buffer to be written.
 * @param len[in] Number of bytes to write.
 *
 * @retval 0 Success: Data written successfully.
 * @retval -ENODEV Error: handle is NULL or not ready.
 * @retval -ENOTSUP Error: channel was opened with tx_pin == ZBOOK_UART_PIN_NONE.
 */
int zbook_uart_write(struct zbook_uart_channel *handle, const uint8_t *data, size_t len);

/**
 * @brief Reads data from an open zbook UART channel.
 *
 * @param handle[in] Channel to read from.
 * @param data[out] Buffer to store the read bytes.
 * @param len[in] Number of bytes to read.
 *
 * @retval 0 Success: Data read successfully.
 * @retval -1 Warning: No characters available to read.
 * @retval -ENODEV Error: handle is NULL or not ready.
 * @retval -EILSEQ Error: parity check failed on a received byte (only
 * possible when the channel was opened with parity != @p ZBOOK_UART_PARITY_NONE).
 * @retval -ENOTSUP Error: channel was opened with rx_pin == ZBOOK_UART_PIN_NONE.
 */
int zbook_uart_read(struct zbook_uart_channel *handle, uint8_t *data, size_t len);

/**
 * @brief Registers (or clears) a callback fired for every byte received on
 * a channel, from interrupt context. Keep it short: no blocking calls.
 *
 * @param handle[in] Channel to set the callback on.
 * @param cb[in] Callback to invoke per received byte, or NULL to disable.
 * Not called at all for a byte that fails the parity check (see
 * zbook_uart_read()'s -EILSEQ).
 * @param user_data[in] Opaque pointer passed through to cb.
 *
 * @retval 0 Success.
 * @retval -ENODEV handle is NULL or not open.
 * @retval -ENOTSUP Not supported on this target, or channel was opened with
 * rx_pin == ZBOOK_UART_PIN_NONE.
 */
int zbook_uart_set_rx_callback(struct zbook_uart_channel *handle, zbook_uart_rx_cb_t cb,
			       void *user_data);

#ifdef __cplusplus
}
#endif

#endif /* ZBOOK_UART_H */
