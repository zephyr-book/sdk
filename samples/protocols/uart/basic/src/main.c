/*******************************************************************
 * @file main.c
 *
 * @brief Sample exercising the zbook UART protocol.
 * @author João Matheus Nascimento Dias (joao.dias@edge.ufal.br)
 * @version 0.6
 * @date 25/09/2026
 *
 * @copyright Copyright (c) 2026
 *
 *******************************************************************/

#include "protocols/zbook_uart.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(zbook_uart_sample, LOG_LEVEL_INF);

/* Called from interrupt context every time a byte lands in the RX FIFO. */
static void on_rx_byte(struct zbook_uart_channel *handle, uint8_t byte, void *user_data)
{
	ARG_UNUSED(handle);
	ARG_UNUSED(user_data);

	LOG_INF("uart rx callback: %c", (char)byte);
}

static struct {
	uint32_t tx_pin;
	uint32_t rx_pin;
	uint32_t baudrate;
	zbook_uart_rx_cb_t cb;
} self = {
	.tx_pin = ZBOOK_UART_GPIO_01,
	.rx_pin = ZBOOK_UART_GPIO_39,
	.baudrate = 115200,
	.cb = (zbook_uart_rx_cb_t)on_rx_byte,
};

int main(void)
{
	LOG_INF("Starting zbook UART sample...");

	static const uint8_t tx_data[] = "Hello from zbook UART!";
	struct zbook_uart_channel *uart;



	if (zbook_uart_init(self.tx_pin, self.rx_pin, self.baudrate, &uart) != 0) {
		LOG_ERR("failed to open zbook uart");
		return 0;
	}

	if (zbook_uart_set_rx_callback(uart, self.cb, NULL) != 0) {
		LOG_WRN("rx callback not supported on this target, falling back to polling");
	}

	LOG_INF("zbook UART sample initialized, starting loop...");

	while (1) {
		zbook_uart_write(uart, tx_data, sizeof(tx_data));
		k_sleep(K_SECONDS(1));
	}

	return 0;
}
