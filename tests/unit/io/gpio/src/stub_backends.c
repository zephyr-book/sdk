/*
 * zbook_pin.c links every backend; only the GPIO one is under test here.
 */
#include "io/zbook_pin.h"

#include <errno.h>
#include <zephyr/sys/util.h>

int zbook_io_pwm_init(struct zbook_pin *pin, const struct zbook_pin_header_config *cfg)
{
	ARG_UNUSED(pin);
	ARG_UNUSED(cfg);
	return -ENOTSUP;
}

void zbook_io_pwm_deinit(struct zbook_pin *pin)
{
	ARG_UNUSED(pin);
}

int zbook_io_pwm_set(struct zbook_pin *pin, const void *buf, size_t len)
{
	ARG_UNUSED(pin);
	ARG_UNUSED(buf);
	ARG_UNUSED(len);
	return -ENOTSUP;
}

int zbook_io_adc_init(struct zbook_pin *pin, const struct zbook_pin_header_config *cfg)
{
	ARG_UNUSED(pin);
	ARG_UNUSED(cfg);
	return -ENOTSUP;
}

void zbook_io_adc_deinit(struct zbook_pin *pin)
{
	ARG_UNUSED(pin);
}

int zbook_io_adc_read(struct zbook_pin *pin, void *buf, size_t len)
{
	ARG_UNUSED(pin);
	ARG_UNUSED(buf);
	ARG_UNUSED(len);
	return -ENOTSUP;
}

int zbook_io_uart_init(struct zbook_pin *pin, const struct zbook_pin_header_config *cfg)
{
	ARG_UNUSED(pin);
	ARG_UNUSED(cfg);
	return -ENOTSUP;
}

void zbook_io_uart_deinit(struct zbook_pin *pin)
{
	ARG_UNUSED(pin);
}
