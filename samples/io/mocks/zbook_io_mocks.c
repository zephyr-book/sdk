/*************************************************************************
 * @file zbook_io_mocks.c
 *
 * @brief Mock implementation of the Zbook I/O backends for the samples.
 *
 * @details Provides zbook_io_{gpio,pwm,adc,uart}_*() without touching any
 * hardware: the state lives in the pin context. Init claims the pin and
 * deinit releases it, so an init over a live function fails with -EBUSY.
 * Lets the samples build and run while the real backends are not linked in.
 *
 * @note Do not build it together with the real backends (duplicate symbols):
 * drop it from the sample's CMakeLists.txt once they are available.
 *
 * @author João Matheus Nascimento Dias (joao.dias@edge.ufal.br)
 * @version 0.1
 * @date 20/10/2026
 *
 * @copyright Centro de Inovação EDGE 2026. Todos os direitos reservados.
 *
 ***********************************************************************/

#include "io/zbook_pin.h"

#include <errno.h>
#include <string.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(zbook_io_mocks, LOG_LEVEL_INF);

#define MOCK_ADC_NATIVE_BITS 12
#define MOCK_ADC_STEP        257

/* Per-pin mock state: pin->ctx has no field for the GPIO level or the claim. */
struct mock_pin_state {
	bool claimed;  /**< A backend init succeeded and no deinit followed yet */
	uint8_t level; /**< Last GPIO level written or set at init */
};

static struct mock_pin_state mock_state[ZBOOK_PIN_HEADER_AMOUNT];

static struct mock_pin_state *state_of(const struct zbook_pin *pin)
{
	switch (pin->gpio) {
	case ZBOOK_PIN_HEADER_IO_01:
		return &mock_state[0];
	case ZBOOK_PIN_HEADER_IO_39:
		return &mock_state[1];
	case ZBOOK_PIN_HEADER_IO_45:
		return &mock_state[2];
	default:
		return &mock_state[3];
	}
}

static inline int mock_claim(struct zbook_pin *pin)
{
	struct mock_pin_state *state = state_of(pin);

	if (state->claimed) {
		LOG_ERR("[mock] GPIO%d init without deinit of the previous function", pin->gpio);
		return -EBUSY;
	}

	state->claimed = true;

	return 0;
}

static inline void mock_release(struct zbook_pin *pin)
{
	struct mock_pin_state *state = state_of(pin);

	state->claimed = false;
	state->level = ZBOOK_PIN_HEADER_STATE_LOW;
	memset(&pin->ctx, 0, sizeof(pin->ctx));
}

/**< GPIO */

#ifndef CONFIG_ZBOOK_INTERFACE_IO_GPIO

int zbook_io_gpio_init(struct zbook_pin *pin, const struct zbook_pin_header_config *cfg)
{
	if (pin == NULL || cfg == NULL) {
		return -EINVAL;
	}

	int ret = mock_claim(pin);

	if (ret != 0) {
		return ret;
	}

	pin->ctx.gpio.direction = cfg->cfg.gpio.direction;
	pin->ctx.gpio.open_drain = cfg->cfg.gpio.state == ZBOOK_PIN_HEADER_STATE_OPEN_DRAIN;
	state_of(pin)->level = cfg->cfg.gpio.state == ZBOOK_PIN_HEADER_STATE_HIGH
				       ? ZBOOK_PIN_HEADER_STATE_HIGH
				       : ZBOOK_PIN_HEADER_STATE_LOW;

	LOG_INF("[mock] GPIO%d init: %s", pin->gpio,
		pin->ctx.gpio.direction == ZBOOK_PIN_HEADER_DIRECTION_OUTPUT ? "output" : "input");

	return 0;
}

void zbook_io_gpio_deinit(struct zbook_pin *pin)
{
	mock_release(pin);
}

int zbook_io_gpio_write(struct zbook_pin *pin, const void *buf, size_t len)
{
	if (len != 1) {
		return -EINVAL;
	}

	if (pin->ctx.gpio.direction != ZBOOK_PIN_HEADER_DIRECTION_OUTPUT) {
		return -ENOTSUP;
	}

	state_of(pin)->level = *(const uint8_t *)buf;
	LOG_INF("[mock] GPIO%d write: %u", pin->gpio, state_of(pin)->level);

	return 0;
}

int zbook_io_gpio_read(struct zbook_pin *pin, void *buf, size_t len)
{
	if (len != 1) {
		return -EINVAL;
	}

	*(uint8_t *)buf = state_of(pin)->level;
	LOG_INF("[mock] GPIO%d read: %u", pin->gpio, state_of(pin)->level);

	return 0;
}

#endif /* CONFIG_ZBOOK_INTERFACE_IO_GPIO */

/**< PWM */

int zbook_io_pwm_init(struct zbook_pin *pin, const struct zbook_pin_header_config *cfg)
{
	if (pin == NULL || cfg == NULL || cfg->cfg.pwm.frequency == 0 ||
	    cfg->cfg.pwm.duty_cycle > 100) {
		return -EINVAL;
	}

	int ret = mock_claim(pin);

	if (ret != 0) {
		return ret;
	}

	pin->ctx.pwm.polarity = cfg->cfg.pwm.polarity;
	pin->ctx.pwm.wrap = 0;

	LOG_INF("[mock] GPIO%d PWM init: %u Hz, duty %u%%", pin->gpio, cfg->cfg.pwm.frequency,
		cfg->cfg.pwm.duty_cycle);

	return 0;
}

void zbook_io_pwm_deinit(struct zbook_pin *pin)
{
	mock_release(pin);
}

int zbook_io_pwm_set(struct zbook_pin *pin, const void *buf, size_t len)
{
	if (len != 1 || *(const uint8_t *)buf > 100) {
		return -EINVAL;
	}

	LOG_INF("[mock] GPIO%d PWM duty: %u%%", pin->gpio, *(const uint8_t *)buf);

	return 0;
}

/**< ADC */

int zbook_io_adc_init(struct zbook_pin *pin, const struct zbook_pin_header_config *cfg)
{
	if (pin == NULL || cfg == NULL || cfg->cfg.adc.resolution > MOCK_ADC_NATIVE_BITS) {
		return -EINVAL;
	}

	int ret = mock_claim(pin);

	if (ret != 0) {
		return ret;
	}

	pin->ctx.adc.channel = pin->gpio - 40;
	pin->ctx.adc.resolution =
		cfg->cfg.adc.resolution != 0 ? cfg->cfg.adc.resolution : MOCK_ADC_NATIVE_BITS;

	LOG_INF("[mock] GPIO%d ADC init: %u bits", pin->gpio, pin->ctx.adc.resolution);

	return 0;
}

void zbook_io_adc_deinit(struct zbook_pin *pin)
{
	mock_release(pin);
}

int zbook_io_adc_read(struct zbook_pin *pin, void *buf, size_t len)
{
	static uint16_t raw;

	if (len != sizeof(uint16_t)) {
		return -EINVAL;
	}

	/* Ramp through the full scale so consecutive samples differ. */
	raw = (raw + MOCK_ADC_STEP) & (BIT(MOCK_ADC_NATIVE_BITS) - 1);

	uint16_t sample = raw >> (MOCK_ADC_NATIVE_BITS - pin->ctx.adc.resolution);

	memcpy(buf, &sample, sizeof(sample));

	return 0;
}

/**< UART */

int zbook_io_uart_init(struct zbook_pin *pin, const struct zbook_pin_header_config *cfg)
{
	if (pin == NULL || cfg == NULL || cfg->cfg.uart.baudrate == 0) {
		return -EINVAL;
	}

	int ret = mock_claim(pin);

	if (ret != 0) {
		return ret;
	}

	pin->ctx.uart.role = cfg->cfg.uart.role;
	pin->ctx.uart.baudrate = cfg->cfg.uart.baudrate;

	LOG_INF("[mock] GPIO%d UART init: %s, %u baud", pin->gpio,
		pin->ctx.uart.role == ZBOOK_PIN_HEADER_UART_ROLE_TX ? "TX" : "RX",
		pin->ctx.uart.baudrate);

	return 0;
}

void zbook_io_uart_deinit(struct zbook_pin *pin)
{
	mock_release(pin);
}
