/*******************************************************************
 * @file main.c
 *
 * @brief Unit tests for the zbook UART protocol interface. Runs on
 * native_sim against zephyr,uart-emul stand-ins for the real PIO UARTs
 * (see app.overlay), each tagged with the TX/RX pin pair it represents --
 * exercises every branch of zbook_uart.c: unknown pin pair, not-ready-
 * before-open, open/already-ready, write+read over loopback, read-on-empty,
 * and the device_init()-failed mapping to -EBUSY.
 * @author João Matheus Nascimento Dias (joao.dias@edge.ufal.br)
 * @version 0.2
 * @date 25/09/2026
 *
 * @copyright Copyright (c) 2026
 *
 *******************************************************************/

#include <protocols/zbook_uart.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/ztest.h>

/* Matches app.overlay's uart_emul_a. */
#define UART_A_TX 45
#define UART_A_RX 46

/* Matches app.overlay's uart_emul_b. */
#define UART_B_TX 1
#define UART_B_RX 39

/* Matches app.overlay's uart_fail_c -- not a real pin pair, just a tag. */
#define UART_FAIL_TX 98
#define UART_FAIL_RX 99

/* No channel is wired up for this pin pair -- exercises the not-found case. */
#define UART_UNKNOWN_TX 254
#define UART_UNKNOWN_RX 255

/*
* A 3rd zbook_uart_bus child (app.overlay) whose init always fails, wired up
* here (not via a real driver) purely to exercise zbook_uart_init()'s
* device_init()-failed branch -- no real UART backend fails init, so
* there's no other way to reach it.
*/
#define DT_DRV_COMPAT vnd_zbook_test_fail_uart

static int fail_uart_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	return -EIO;
}

DEVICE_DT_INST_DEFINE(0, fail_uart_init, NULL, NULL, NULL, POST_KERNEL,
		       CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);

ZTEST_SUITE(zbook_uart, NULL, NULL, NULL, NULL, NULL);

ZTEST(zbook_uart, test_01_unknown_pin_pair)
{
	struct zbook_uart_channel *handle;
	uint8_t byte;

	zassert_equal(zbook_uart_init(UART_UNKNOWN_TX, UART_UNKNOWN_RX, 115200, &handle),
		      -ENODEV);
	zassert_equal(zbook_uart_write(NULL, &byte, 1), -ENODEV);
	zassert_equal(zbook_uart_read(NULL, &byte, 1), -ENODEV);
}

ZTEST(zbook_uart, test_02_close_null_handle_is_rejected)
{
	zassert_equal(zbook_uart_close(NULL), -EINVAL);
	zassert_equal(zbook_uart_init(UART_A_TX, UART_A_RX, 115200, NULL), -EINVAL);
}

ZTEST(zbook_uart, test_03_open)
{
	struct zbook_uart_channel *handle_a, *handle_b;

	zassert_equal(zbook_uart_init(UART_A_TX, UART_A_RX, 115200, &handle_a), 0);
	zassert_not_null(handle_a);
	zassert_equal(zbook_uart_init(UART_B_TX, UART_B_RX, 9600, &handle_b), 0);
	zassert_not_null(handle_b);
}

ZTEST(zbook_uart, test_04_open_already_ready_is_a_noop)
{
	struct zbook_uart_channel *handle_a, *handle_b;

	/* device_is_ready() is now true: takes the early-return path, not device_init(). */
	zassert_equal(zbook_uart_init(UART_A_TX, UART_A_RX, 115200, &handle_a), 0);
	zassert_equal(zbook_uart_init(UART_B_TX, UART_B_RX, 9600, &handle_b), 0);
}

ZTEST(zbook_uart, test_05_write_read_loopback)
{
	static const uint8_t tx_data[] = {'z', 'b', 'k'};
	uint8_t rx_data[sizeof(tx_data)] = {0};
	struct zbook_uart_channel *handle_a, *handle_b;

	zassert_equal(zbook_uart_init(UART_A_TX, UART_A_RX, 115200, &handle_a), 0);
	zassert_equal(zbook_uart_write(handle_a, tx_data, sizeof(tx_data)), 0);
	zassert_equal(zbook_uart_read(handle_a, rx_data, sizeof(rx_data)), 0);
	zassert_mem_equal(rx_data, tx_data, sizeof(tx_data));

	zassert_equal(zbook_uart_init(UART_B_TX, UART_B_RX, 9600, &handle_b), 0);
	zassert_equal(zbook_uart_write(handle_b, tx_data, sizeof(tx_data)), 0);
	zassert_equal(zbook_uart_read(handle_b, rx_data, sizeof(rx_data)), 0);
	zassert_mem_equal(rx_data, tx_data, sizeof(tx_data));
}

ZTEST(zbook_uart, test_06_read_empty_returns_minus_one)
{
	struct zbook_uart_channel *handle_a, *handle_b;
	uint8_t byte;

	/* Nothing pending after test_05 drained both loopback buffers. */
	zassert_equal(zbook_uart_init(UART_A_TX, UART_A_RX, 115200, &handle_a), 0);
	zassert_equal(zbook_uart_read(handle_a, &byte, 1), -1);

	zassert_equal(zbook_uart_init(UART_B_TX, UART_B_RX, 9600, &handle_b), 0);
	zassert_equal(zbook_uart_read(handle_b, &byte, 1), -1);
}

ZTEST(zbook_uart, test_07_open_maps_device_init_failure_to_ebusy)
{
	struct zbook_uart_channel *handle;

	/* fail_uart_init() returns -EIO, but zbook_uart_init() maps any device_init()
	 * failure to -EBUSY rather than passing the driver's errno through.
	 */
	zassert_equal(zbook_uart_init(UART_FAIL_TX, UART_FAIL_RX, 115200, &handle), -EBUSY);
}
