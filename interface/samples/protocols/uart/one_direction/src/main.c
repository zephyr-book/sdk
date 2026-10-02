/*******************************************************************
 * @file main.c
 *
 * @brief
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

#define GPIO_AMOUNT 4

LOG_MODULE_REGISTER(zbook_uart_sample, LOG_LEVEL_INF);

static void on_rx_byte(struct zbook_uart_channel *handle, uint8_t byte, void *user_data)
{
	ARG_UNUSED(handle);

	LOG_INF("uart rx callback by %s.\n", (char *)user_data);
}

static struct {
	int gpios[GPIO_AMOUNT];
	char *data[GPIO_AMOUNT];
	struct zbook_uart_channel *uart[GPIO_AMOUNT];
} self = {
	.gpios = {ZBOOK_UART_GPIO_01, ZBOOK_UART_GPIO_39, ZBOOK_UART_GPIO_45, ZBOOK_UART_GPIO_46},
	.data = {"GPIO1", "GPIO39", "GPIO45", "GPIO46"},
};

int main(void)
{
	LOG_INF("Starting zbook UART sample...");

	static const uint8_t tx_data_gpio1[] = "Hello from zbook UART (GPIO1)!";
	static const uint8_t tx_data_gpio39[] = "Hello from zbook UART (GPIO39)!";
	static const uint8_t tx_data_gpio45[] = "Hello from zbook UART (GPIO45)!";
	static const uint8_t tx_data_gpio46[] = "Hello from zbook UART (GPIO46)!";

	for (int i = 0; i < 4; i++) {

#if CONFIG_ZBOOK_UART_SAMPLE_ONE_DIRECTION_IS_TX
		if (zbook_uart_init(self.gpios[i], ZBOOK_UART_PIN_NONE, 115200, &self.uart[i]) !=
		    0) {
			LOG_ERR("failed to open zbook uart on GPIO %d", self.gpios[i]);
			return 0;
		}
#else
		if (zbook_uart_init(ZBOOK_UART_PIN_NONE, self.gpios[i], 115200, &self.uart[i]) !=
		    0) {
			LOG_ERR("failed to open zbook uart on GPIO %d", self.gpios[i]);
			return 0;
		}

		if (zbook_uart_set_rx_callback(self.uart[i], on_rx_byte, (void *)self.data[i]) != 0) {
			LOG_WRN("rx callback not supported on this target, falling back to "
				"polling");
		}
#endif
	}

#if CONFIG_ZBOOK_UART_SAMPLE_ONE_DIRECTION_IS_TX

	while (1) {
		zbook_uart_write(self.uart[0], tx_data_gpio1, sizeof(tx_data_gpio1));
		zbook_uart_write(self.uart[1], tx_data_gpio39, sizeof(tx_data_gpio39));
		zbook_uart_write(self.uart[2], tx_data_gpio45, sizeof(tx_data_gpio45));
		zbook_uart_write(self.uart[3], tx_data_gpio46, sizeof(tx_data_gpio46));
		k_sleep(K_SECONDS(1));
	}

#endif

	return 0;
}
