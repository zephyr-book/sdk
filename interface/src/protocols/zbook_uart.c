/*******************************************************************
 * @file zbook_uart.c
 *
 * @brief Implementation of the Zbook UART protocol interface.
 * @author João Matheus Nascimento Dias (joao.dias@edge.ufal.br)
 * @version 0.5
 * @date 25/09/2026
 *
 * @copyright Copyright (c) 2026
 *
 *******************************************************************/

#include "protocols/zbook_uart.h"

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(zbook_uart, CONFIG_ZBOOK_INTERFACE_PROTOCOLS_UART_LOG_LEVEL);

#define ZBOOK_UART_MAX_CHANNELS 4
#define ZBOOK_UART_MIN_DATA_BITS 5
#define ZBOOK_UART_MAX_DATA_BITS 9

static bool data_bits_valid(uint8_t data_bits)
{
	return data_bits >= ZBOOK_UART_MIN_DATA_BITS && data_bits <= ZBOOK_UART_MAX_DATA_BITS;
}

#if DT_HAS_COMPAT_STATUS_OKAY(raspberrypi_pico_pio)

#include <zephyr/drivers/misc/pio_rpi_pico/pio_rpi_pico.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>

#include <hardware/clocks.h>
#include <hardware/gpio.h>
#include <hardware/pio.h>
#include <hardware/pio_instructions.h>

#define CYCLES_PER_BIT 8
#define SIDESET_BIT_COUNT 2

#define ZBOOK_UART_TX_PROGRAM_LEN 4
#define ZBOOK_UART_RX_PROGRAM_LEN 9

static struct zbook_uart_channel channel_pool[ZBOOK_UART_MAX_CHANNELS];

#define ZBOOK_UART_PIO_BLOCK(node_id) DEVICE_DT_GET(node_id),

static const struct device *pio_blocks[] = {
	DT_FOREACH_STATUS_OKAY(raspberrypi_pico_pio, ZBOOK_UART_PIO_BLOCK)};

static struct zbook_uart_channel *alloc_channel(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(channel_pool); i++) {
		if (!channel_pool[i].in_use) {
			channel_pool[i].in_use = true;
			return &channel_pool[i];
		}
	}

	return NULL;
}

#if PICO_PIO_USE_GPIO_BASE
static uint32_t gpio_base_for_pin(uint32_t pin)
{
	return (pin >= 32) ? 16 : 0;
}
#else
static uint32_t gpio_base_for_pin(uint32_t pin)
{
	ARG_UNUSED(pin);
	return 0;
}
#endif

static uint8_t frame_bits(uint8_t data_bits, uint8_t parity)
{
	return data_bits + (parity == ZBOOK_UART_PARITY_NONE ? 0 : 1);
}

static bool even_parity_of(uint32_t data, uint8_t data_bits)
{
	return __builtin_parity(data & (BIT(data_bits) - 1)) != 0;
}

/*
 * Same shift loop as a plain "shift out total_bits, side-set the line low
 * for the start bit, high the rest of the time" bit-banged UART TX -- only
 * the loop count (x) varies with data_bits/parity.
 *
 *	0: pull   block           side 1 [7]
 *	1: set    x, total_bits-1 side 0 [7]
 *	2: out    pins, 1
 *	3: jmp    x--, 2                 [6]
 */
static void build_tx_program(uint16_t instr[static ZBOOK_UART_TX_PROGRAM_LEN], uint8_t total_bits)
{
	/*
	 * sm_config_set_sideset() is called with SIDESET_BIT_COUNT=2 (1
	 * output bit + 1 stolen for the optional-sideset enable flag), but
	 * pio_encode_sideset_opt()'s own count parameter is only the output
	 * bit width (the encoder adds the enable bit itself) -- so this is
	 * 1, not SIDESET_BIT_COUNT.
	 */
	instr[0] = pio_encode_pull(false, true) | pio_encode_sideset_opt(1, 1) |
		   pio_encode_delay(CYCLES_PER_BIT - 1);
	instr[1] = pio_encode_set(pio_x, total_bits - 1) |
		   pio_encode_sideset_opt(1, 0) | pio_encode_delay(CYCLES_PER_BIT - 1);
	instr[2] = pio_encode_out(pio_pins, 1);
	instr[3] = pio_encode_jmp_x_dec(2) | pio_encode_delay(CYCLES_PER_BIT - 2);
}

/*
 * Same start-bit-detect + shift-in loop as a plain bit-banged UART RX --
 * only the loop count (x) varies with data_bits/parity.
 *
 *	0: wait   1 pin, 0
 *	1: wait   0 pin, 0
 *	2: set    x, total_bits-1  [10]
 *	3: in     pins, 1
 *	4: jmp    x--, 3           [6]
 *	5: jmp    pin, 8
 *	6: irq    nowait 4 rel
 *	7: jmp    0
 *	8: push   block
 */
static void build_rx_program(uint16_t instr[static ZBOOK_UART_RX_PROGRAM_LEN], uint8_t total_bits)
{
	instr[0] = pio_encode_wait_pin(true, 0);
	instr[1] = pio_encode_wait_pin(false, 0);
	instr[2] = pio_encode_set(pio_x, total_bits - 1) | pio_encode_delay(10);
	instr[3] = pio_encode_in(pio_pins, 1);
	instr[4] = pio_encode_jmp_x_dec(3) | pio_encode_delay(CYCLES_PER_BIT - 2);
	instr[5] = pio_encode_jmp_pin(8);
	instr[6] = pio_encode_irq_set(true, 4);
	instr[7] = pio_encode_jmp(0);
	instr[8] = pio_encode_push(false, true);
}

static int zbook_pio_tx_init(PIO pio, uint32_t sm, uint32_t tx_pin, float div, uint8_t total_bits,
			      uint32_t *offset)
{
	uint16_t instructions[ZBOOK_UART_TX_PROGRAM_LEN];

	build_tx_program(instructions, total_bits);

	const struct pio_program program = {
		.instructions = instructions,
		.length = ZBOOK_UART_TX_PROGRAM_LEN,
		.origin = -1,
	};

	if (!pio_can_add_program(pio, &program)) {
		return -EBUSY;
	}

	*offset = pio_add_program(pio, &program);

	pio_sm_config sm_config = pio_get_default_sm_config();

	sm_config_set_sideset(&sm_config, SIDESET_BIT_COUNT, true, false);
	sm_config_set_out_shift(&sm_config, true, false, 0);
	sm_config_set_out_pins(&sm_config, tx_pin, 1);
	sm_config_set_sideset_pins(&sm_config, tx_pin);
	sm_config_set_fifo_join(&sm_config, PIO_FIFO_JOIN_TX);
	sm_config_set_clkdiv(&sm_config, div);
	sm_config_set_wrap(&sm_config, *offset, *offset + ZBOOK_UART_TX_PROGRAM_LEN - 1);

	pio_sm_set_pins_with_mask64(pio, sm, BIT64(tx_pin), BIT64(tx_pin));
	pio_sm_set_pindirs_with_mask64(pio, sm, BIT64(tx_pin), BIT64(tx_pin));

	int ret = pio_sm_init(pio, sm, *offset, &sm_config);

	if (ret < 0) {
		pio_remove_program(pio, &program, *offset);
		return ret;
	}

	pio_gpio_init(pio, tx_pin);
	pio_sm_set_enabled(pio, sm, true);

	return 0;
}

static int zbook_pio_rx_init(PIO pio, uint32_t sm, uint32_t rx_pin, float div, uint8_t total_bits,
			      uint32_t *offset)
{
	uint16_t instructions[ZBOOK_UART_RX_PROGRAM_LEN];

	build_rx_program(instructions, total_bits);

	const struct pio_program program = {
		.instructions = instructions,
		.length = ZBOOK_UART_RX_PROGRAM_LEN,
		.origin = -1,
	};

	if (!pio_can_add_program(pio, &program)) {
		return -EBUSY;
	}

	*offset = pio_add_program(pio, &program);

	pio_sm_config sm_config = pio_get_default_sm_config();

	pio_sm_set_consecutive_pindirs(pio, sm, rx_pin, 1, false);
	sm_config_set_in_pins(&sm_config, rx_pin);
	sm_config_set_jmp_pin(&sm_config, rx_pin);
	sm_config_set_in_shift(&sm_config, true, false, 0);
	sm_config_set_fifo_join(&sm_config, PIO_FIFO_JOIN_RX);
	sm_config_set_clkdiv(&sm_config, div);
	sm_config_set_wrap(&sm_config, *offset + 1, *offset + ZBOOK_UART_RX_PROGRAM_LEN - 1);

	int ret = pio_sm_init(pio, sm, *offset, &sm_config);

	if (ret < 0) {
		pio_remove_program(pio, &program, *offset);
		return ret;
	}

	pio_gpio_init(pio, rx_pin);
	gpio_set_input_enabled(rx_pin, true);
	gpio_pull_up(rx_pin);
	pio_sm_set_enabled(pio, sm, true);

	return 0;
}

/*
 * Finds a PIO block that can address `pin`, claims a state machine on it,
 * and (on chips that need it) sets its gpio_base to the window `pin` needs.
 * An already-claimed block is only reused if its gpio_base already matches
 * -- so a pin outside every currently-open channel's window lands on a
 * fresh block instead of stealing/breaking one that's already serving a
 * different window. This is what lets a single channel's TX and RX sit on
 * different PIO blocks when their pins need different windows (e.g. TX on
 * a low GPIO, RX on one >=32 on RP2350B).
 */
static int claim_sm_for_pin(uint32_t pin, const struct device **out_dev, PIO *out_pio,
			     size_t *out_sm)
{
	uint32_t gpio_base = gpio_base_for_pin(pin);

	for (size_t i = 0; i < ARRAY_SIZE(pio_blocks); i++) {
		const struct device *dev = pio_blocks[i];

		if (!device_is_ready(dev)) {
			continue;
		}

		PIO pio = pio_rpi_pico_get_pio(dev);
		bool block_in_use = false;

		for (uint sm = 0; sm < NUM_PIO_STATE_MACHINES; sm++) {
			if (pio_sm_is_claimed(pio, sm)) {
				block_in_use = true;
				break;
			}
		}

		if (!block_in_use) {
			/*
			 * pico-sdk's own program-space bookkeeping
			 * (_used_instruction_space) is a plain static, and this
			 * SoC's SRAM isn't guaranteed zeroed by the time it's first
			 * read after a debug-probe reset -- clear the block's
			 * instruction memory (and that bookkeeping) before trusting
			 * it's actually free, or pio_set_gpio_base()/pio_add_program()
			 * can spuriously refuse a genuinely unused block.
			 */
			pio_clear_instruction_memory(pio);
		}

#if PICO_PIO_USE_GPIO_BASE
		if (block_in_use) {
			if (pio_get_gpio_base(pio) != gpio_base) {
				continue;
			}
			/*
			 * Already at the right base -- pio_set_gpio_base() must
			 * NOT be called again here: pico-sdk refuses to touch
			 * gpio_base (even to the same value) once any program is
			 * loaded in the block's instruction memory, which is
			 * always true for a block already serving another
			 * channel.
			 */
		} else if (pio_set_gpio_base(pio, gpio_base) < 0) {
			continue;
		}
#endif

		size_t sm;

		if (pio_rpi_pico_allocate_sm(dev, &sm) < 0) {
			continue;
		}

		*out_dev = dev;
		*out_pio = pio;
		*out_sm = sm;
		return 0;
	}

	return -ENODEV;
}

int zbook_uart_configure(const struct zbook_uart_config *config, struct zbook_uart_channel **handle)
{
	if (config == NULL || handle == NULL) {
		return -EINVAL;
	}

	if (!data_bits_valid(config->data_bits)) {
		LOG_ERR("data_bits %u out of range [%u, %u]", config->data_bits,
			ZBOOK_UART_MIN_DATA_BITS, ZBOOK_UART_MAX_DATA_BITS);
		return -EINVAL;
	}

	bool want_tx = config->tx_pin != ZBOOK_UART_PIN_NONE;
	bool want_rx = config->rx_pin != ZBOOK_UART_PIN_NONE;

	if (!want_tx && !want_rx) {
		LOG_ERR("tx_pin and rx_pin can't both be ZBOOK_UART_PIN_NONE");
		return -EINVAL;
	}

	struct zbook_uart_channel *ch = alloc_channel();

	if (ch == NULL) {
		return -EBUSY;
	}

	PIO tx_pio = NULL;
	PIO rx_pio = NULL;
	size_t tx_sm = 0;
	size_t rx_sm = 0;
	const struct device *tx_dev, *rx_dev;
	int ret;

	if (want_tx) {
		ret = claim_sm_for_pin(config->tx_pin, &tx_dev, &tx_pio, &tx_sm);
		if (ret < 0) {
			ch->in_use = false;
			LOG_ERR("no PIO block could serve tx pin %u (%d)", config->tx_pin, ret);
			return ret;
		}
	}

	if (want_rx) {
		ret = claim_sm_for_pin(config->rx_pin, &rx_dev, &rx_pio, &rx_sm);
		if (ret < 0) {
			if (want_tx) {
				pio_sm_unclaim(tx_pio, tx_sm);
			}
			ch->in_use = false;
			LOG_ERR("no PIO block could serve rx pin %u (%d)", config->rx_pin, ret);
			return ret;
		}
	}

	uint8_t total_bits = frame_bits(config->data_bits, config->parity);
	float div = (float)clock_get_hz(clk_sys) / (CYCLES_PER_BIT * config->baudrate);

	if (want_tx) {
		ret = zbook_pio_tx_init(tx_pio, tx_sm, config->tx_pin, div, total_bits,
					 &ch->tx_offset);
		if (ret < 0) {
			pio_sm_unclaim(tx_pio, tx_sm);
			if (want_rx) {
				pio_sm_unclaim(rx_pio, rx_sm);
			}
			ch->in_use = false;
			return ret;
		}
	}

	if (want_rx) {
		ret = zbook_pio_rx_init(rx_pio, rx_sm, config->rx_pin, div, total_bits,
					 &ch->rx_offset);
		if (ret < 0) {
			if (want_tx) {
				const struct pio_program tx_program = {
					.length = ZBOOK_UART_TX_PROGRAM_LEN};

				pio_sm_set_enabled(tx_pio, tx_sm, false);
				pio_remove_program(tx_pio, &tx_program, ch->tx_offset);
				pio_sm_unclaim(tx_pio, tx_sm);
			}
			pio_sm_unclaim(rx_pio, rx_sm);
			ch->in_use = false;
			return ret;
		}
	}

	ch->tx_pio = tx_pio;
	ch->rx_pio = rx_pio;
	ch->tx_sm = tx_sm;
	ch->rx_sm = rx_sm;
	ch->data_bits = config->data_bits;
	ch->parity = config->parity;
	*handle = ch;

	return 0;
}

int zbook_uart_init(uint32_t tx_pin, uint32_t rx_pin, uint32_t baudrate,
		     struct zbook_uart_channel **handle)
{
	struct zbook_uart_config config = {
		.tx_pin = tx_pin,
		.rx_pin = rx_pin,
		.baudrate = baudrate,
		.data_bits = 8,
		.parity = ZBOOK_UART_PARITY_NONE,
	};

	return zbook_uart_configure(&config, handle);
}

int zbook_uart_close(struct zbook_uart_channel *handle)
{
	if (handle == NULL || !handle->in_use) {
		return -EINVAL;
	}

	const struct pio_program tx_program = {.length = ZBOOK_UART_TX_PROGRAM_LEN};
	const struct pio_program rx_program = {.length = ZBOOK_UART_RX_PROGRAM_LEN};

	if (handle->rx_pio != NULL) {
		pio_set_irq0_source_enabled(
			handle->rx_pio, pio_get_rx_fifo_not_empty_interrupt_source(handle->rx_sm),
			false);
		handle->rx_cb = NULL;

		pio_sm_set_enabled(handle->rx_pio, handle->rx_sm, false);
		pio_remove_program(handle->rx_pio, &rx_program, handle->rx_offset);
		pio_sm_unclaim(handle->rx_pio, handle->rx_sm);
	}

	if (handle->tx_pio != NULL) {
		pio_sm_set_enabled(handle->tx_pio, handle->tx_sm, false);
		pio_remove_program(handle->tx_pio, &tx_program, handle->tx_offset);
		pio_sm_unclaim(handle->tx_pio, handle->tx_sm);
	}

	handle->in_use = false;

	return 0;
}

int zbook_uart_write(struct zbook_uart_channel *handle, const uint8_t *data, size_t len)
{
	if (handle == NULL || !handle->in_use) {
		return -ENODEV;
	}

	if (handle->tx_pio == NULL) {
		return -ENOTSUP;
	}

	for (size_t i = 0; i < len; i++) {
		uint32_t word = data[i] & (BIT(handle->data_bits) - 1);

		if (handle->parity != ZBOOK_UART_PARITY_NONE) {
			bool even = even_parity_of(word, handle->data_bits);
			bool parity_bit = (handle->parity == ZBOOK_UART_PARITY_EVEN) ? even
										       : !even;

			word |= (uint32_t)parity_bit << handle->data_bits;
		}

		pio_sm_put_blocking(handle->tx_pio, handle->tx_sm, word);
	}

	return 0;
}

static bool pop_rx_byte(struct zbook_uart_channel *handle, uint8_t *out)
{
	uint8_t total_bits = frame_bits(handle->data_bits, handle->parity);
	uint32_t raw = handle->rx_pio->rxf[handle->rx_sm];
	uint32_t justified = raw >> (32 - total_bits);
	uint32_t value = justified & (BIT(handle->data_bits) - 1);

	if (handle->parity != ZBOOK_UART_PARITY_NONE) {
		bool received_parity = (justified >> handle->data_bits) & 1;
		bool even = even_parity_of(value, handle->data_bits);
		bool expected_parity = (handle->parity == ZBOOK_UART_PARITY_EVEN) ? even : !even;

		if (received_parity != expected_parity) {
			return false;
		}
	}

	*out = (uint8_t)value;
	return true;
}

int zbook_uart_read(struct zbook_uart_channel *handle, uint8_t *data, size_t len)
{
	if (handle == NULL || !handle->in_use) {
		return -ENODEV;
	}

	if (handle->rx_pio == NULL) {
		return -ENOTSUP;
	}

	for (size_t i = 0; i < len; i++) {
		if (pio_sm_is_rx_fifo_empty(handle->rx_pio, handle->rx_sm)) {
			return -1;
		}

		if (!pop_rx_byte(handle, &data[i])) {
			LOG_ERR("parity error on byte %zu", i);
			return -EILSEQ;
		}
	}

	return 0;
}

static void zbook_uart_pio_isr(const struct device *piodev)
{
	PIO pio = pio_rpi_pico_get_pio(piodev);
	uint32_t ints = pio->ints0;

	for (uint sm = 0; sm < NUM_PIO_STATE_MACHINES; sm++) {
		if (!(ints & BIT(pis_sm0_rx_fifo_not_empty + sm))) {
			continue;
		}

		for (size_t i = 0; i < ARRAY_SIZE(channel_pool); i++) {
			struct zbook_uart_channel *ch = &channel_pool[i];

			if (!ch->in_use || ch->rx_pio != pio || ch->rx_sm != sm ||
			    ch->rx_cb == NULL) {
				continue;
			}

			while (!pio_sm_is_rx_fifo_empty(pio, sm)) {
				uint8_t byte;

				if (pop_rx_byte(ch, &byte)) {
					ch->rx_cb(ch, byte, ch->rx_user_data);
				}
			}
		}
	}
}

#define ZBOOK_UART_PIO_IRQ_CONNECT(node_id)                                                       \
	IRQ_CONNECT(DT_IRQ_BY_NAME(node_id, irq0, irq), DT_IRQ_BY_NAME(node_id, irq0, priority),  \
		    zbook_uart_pio_isr, DEVICE_DT_GET(node_id), 0);                               \
	irq_enable(DT_IRQ_BY_NAME(node_id, irq0, irq));

static int zbook_uart_pio_irq_init(void)
{
	DT_FOREACH_STATUS_OKAY(raspberrypi_pico_pio, ZBOOK_UART_PIO_IRQ_CONNECT)

	return 0;
}

SYS_INIT(zbook_uart_pio_irq_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

int zbook_uart_set_rx_callback(struct zbook_uart_channel *handle,
				void (*cb)(struct zbook_uart_channel *handle, uint8_t byte,
					   void *user_data),
				void *user_data)
{
	if (handle == NULL || !handle->in_use) {
		return -ENODEV;
	}

	if (handle->rx_pio == NULL) {
		return -ENOTSUP;
	}

	/* Disable the source before touching the callback fields, then only
	 * re-enable (with the new fields already in place) if cb != NULL --
	 * an in-flight ISR must never see a callback without its user_data,
	 * or vice versa.
	 */
	pio_set_irq0_source_enabled(handle->rx_pio,
				     pio_get_rx_fifo_not_empty_interrupt_source(handle->rx_sm),
				     false);

	handle->rx_user_data = user_data;
	handle->rx_cb = cb;

	if (cb != NULL) {
		pio_set_irq0_source_enabled(
			handle->rx_pio, pio_get_rx_fifo_not_empty_interrupt_source(handle->rx_sm),
			true);
	}

	return 0;
}

#else /* !DT_HAS_COMPAT_STATUS_OKAY(raspberrypi_pico_pio) */

#include <zephyr/drivers/uart.h>

#define DT_DRV_COMPAT vnd_zbook_uart_emul

#define ZBOOK_UART_EMUL_ENTRY(node_id)                                                         \
	{                                                                                          \
		.dev = DEVICE_DT_GET(node_id),                                                     \
		.tx_pin = DT_PROP(node_id, zbook_tx_pin),                                          \
		.rx_pin = DT_PROP(node_id, zbook_rx_pin),                                          \
	},

static struct zbook_uart_channel channels[] = {DT_FOREACH_STATUS_OKAY(DT_DRV_COMPAT,
									ZBOOK_UART_EMUL_ENTRY)};

static struct zbook_uart_channel *find_channel(uint32_t tx_pin, uint32_t rx_pin)
{
	for (size_t i = 0; i < ARRAY_SIZE(channels); i++) {
		if (channels[i].tx_pin == tx_pin && channels[i].rx_pin == rx_pin) {
			return &channels[i];
		}
	}

	return NULL;
}

int zbook_uart_configure(const struct zbook_uart_config *config, struct zbook_uart_channel **handle)
{
	if (config == NULL || handle == NULL) {
		return -EINVAL;
	}

	if (!data_bits_valid(config->data_bits)) {
		LOG_ERR("data_bits %u out of range [%u, %u]", config->data_bits,
			ZBOOK_UART_MIN_DATA_BITS, ZBOOK_UART_MAX_DATA_BITS);
		return -EINVAL;
	}

	struct zbook_uart_channel *ch = find_channel(config->tx_pin, config->rx_pin);

	if (ch == NULL) {
		LOG_ERR("no zbook_uart emul channel wired up for tx=%u rx=%u", config->tx_pin,
			config->rx_pin);
		return -ENODEV;
	}

	if (!device_is_ready(ch->dev)) {
		int ret = device_init(ch->dev);

		if (ret < 0) {
			LOG_ERR("zbook_uart tx=%u rx=%u init failed (%d)", config->tx_pin,
				config->rx_pin, ret);
			return -EBUSY;
		}
	}

	*handle = ch;

	return 0;
}

int zbook_uart_init(uint32_t tx_pin, uint32_t rx_pin, uint32_t baudrate,
		     struct zbook_uart_channel **handle)
{
	struct zbook_uart_config config = {
		.tx_pin = tx_pin,
		.rx_pin = rx_pin,
		.baudrate = baudrate,
		.data_bits = 8,
		.parity = ZBOOK_UART_PARITY_NONE,
	};

	return zbook_uart_configure(&config, handle);
}

int zbook_uart_close(struct zbook_uart_channel *handle)
{
	if (handle == NULL) {
		return -EINVAL;
	}

	/* Deferred-init devices have no runtime teardown in this Zephyr version. */
	return 0;
}

int zbook_uart_write(struct zbook_uart_channel *handle, const uint8_t *data, size_t len)
{
	if (handle == NULL || !device_is_ready(handle->dev)) {
		return -ENODEV;
	}

	for (size_t i = 0; i < len; i++) {
		uart_poll_out(handle->dev, data[i]);
	}

	return 0;
}

int zbook_uart_read(struct zbook_uart_channel *handle, uint8_t *data, size_t len)
{
	if (handle == NULL || !device_is_ready(handle->dev)) {
		return -ENODEV;
	}

	for (size_t i = 0; i < len; i++) {
		if (uart_poll_in(handle->dev, &data[i]) != 0) {
			return -1;
		}
	}

	return 0;
}

int zbook_uart_set_rx_callback(struct zbook_uart_channel *handle,
				void (*cb)(struct zbook_uart_channel *handle, uint8_t byte,
					   void *user_data),
				void *user_data)
{
	ARG_UNUSED(handle);
	ARG_UNUSED(cb);
	ARG_UNUSED(user_data);

	return -ENOTSUP;
}

#endif /* DT_HAS_COMPAT_STATUS_OKAY(raspberrypi_pico_pio) */