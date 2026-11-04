/*************************************************************************
 * @file fake_backends.c
 *
 * @brief Fake zbook_io_* backends that record how the pin factory calls them.
 * @author João Matheus Nascimento Dias (joao.dias@edge.ufal.br)
 * @version 0.1
 * @date 20/10/2026
 *
 * @copyright Centro de Inovação EDGE 2026. Todos os direitos reservados.
 *
 ***********************************************************************/

#include "fake_backends.h"

#include <string.h>

static struct fake_backend fake[ZBOOK_PIN_HEADER_FUNCTION_AMOUNT];
static struct fake_event fake_log[FAKE_LOG_SIZE];
static int fake_log_len;

struct fake_backend *fake_backend(enum zbook_pin_header_function function)
{
	return &fake[function];
}

int fake_event_count(void)
{
	return fake_log_len;
}

const struct fake_event *fake_event(int idx)
{
	return idx >= 0 && idx < fake_log_len ? &fake_log[idx] : NULL;
}

void fake_reset(void)
{
	memset(fake, 0, sizeof(fake));
	memset(fake_log, 0, sizeof(fake_log));
	fake_log_len = 0;
}

static void record(enum zbook_pin_header_function function, enum fake_op op)
{
	if (fake_log_len < FAKE_LOG_SIZE) {
		fake_log[fake_log_len++] = (struct fake_event){.function = function, .op = op};
	}
}

static int fake_init(enum zbook_pin_header_function function)
{
	record(function, FAKE_OP_INIT);
	fake[function].init_calls++;

	return fake[function].init_ret;
}

static void fake_deinit(enum zbook_pin_header_function function)
{
	record(function, FAKE_OP_DEINIT);
	fake[function].deinit_calls++;
}

static int fake_read(enum zbook_pin_header_function function, void *buf, size_t len)
{
	record(function, FAKE_OP_READ);
	fake[function].read_calls++;
	fake[function].last_len = len;

	if (fake[function].rw_ret == 0) {
		memset(buf, 0, len);
		*(uint8_t *)buf = fake[function].rd_val;
	}

	return fake[function].rw_ret;
}

static int fake_write(enum zbook_pin_header_function function, const void *buf, size_t len)
{
	record(function, FAKE_OP_WRITE);
	fake[function].write_calls++;
	fake[function].last_len = len;
	fake[function].last_wr = *(const uint8_t *)buf;

	return fake[function].rw_ret;
}

/* GPIO */

int zbook_io_gpio_init(struct zbook_pin *pin, const struct zbook_pin_header_config *cfg)
{
	return fake_init(ZBOOK_PIN_HEADER_FUNCTION_GPIO);
}

void zbook_io_gpio_deinit(struct zbook_pin *pin)
{
	fake_deinit(ZBOOK_PIN_HEADER_FUNCTION_GPIO);
}

int zbook_io_gpio_read(struct zbook_pin *pin, void *buf, size_t len)
{
	return fake_read(ZBOOK_PIN_HEADER_FUNCTION_GPIO, buf, len);
}

int zbook_io_gpio_write(struct zbook_pin *pin, const void *buf, size_t len)
{
	return fake_write(ZBOOK_PIN_HEADER_FUNCTION_GPIO, buf, len);
}

/* PWM (write-only) */

int zbook_io_pwm_init(struct zbook_pin *pin, const struct zbook_pin_header_config *cfg)
{
	return fake_init(ZBOOK_PIN_HEADER_FUNCTION_PWM);
}

void zbook_io_pwm_deinit(struct zbook_pin *pin)
{
	fake_deinit(ZBOOK_PIN_HEADER_FUNCTION_PWM);
}

int zbook_io_pwm_set(struct zbook_pin *pin, const void *buf, size_t len)
{
	return fake_write(ZBOOK_PIN_HEADER_FUNCTION_PWM, buf, len);
}

/* ADC (read-only) */

int zbook_io_adc_init(struct zbook_pin *pin, const struct zbook_pin_header_config *cfg)
{
	return fake_init(ZBOOK_PIN_HEADER_FUNCTION_ADC);
}

void zbook_io_adc_deinit(struct zbook_pin *pin)
{
	fake_deinit(ZBOOK_PIN_HEADER_FUNCTION_ADC);
}

int zbook_io_adc_read(struct zbook_pin *pin, void *buf, size_t len)
{
	return fake_read(ZBOOK_PIN_HEADER_FUNCTION_ADC, buf, len);
}

/* UART (no read/write yet) */

int zbook_io_uart_init(struct zbook_pin *pin, const struct zbook_pin_header_config *cfg)
{
	return fake_init(ZBOOK_PIN_HEADER_FUNCTION_UART);
}

void zbook_io_uart_deinit(struct zbook_pin *pin)
{
	fake_deinit(ZBOOK_PIN_HEADER_FUNCTION_UART);
}
