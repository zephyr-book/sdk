/*************************************************************************
 * @file zbook_io_gpio.c
 *
 * @brief Implementation of the Zbook GPIO backend.
 * @author João Matheus Nascimento Dias (joao.dias@edge.ufal.br)
 * @version 0.1
 * @date 09/10/2026
 *
 * @copyright Centro de Inovação EDGE 2026. Todos os direitos reservados.
 *
 ***********************************************************************/

#include "io/zbook_io_gpio.h"

#include <errno.h>
#include <zephyr/sys/util.h>

#include <hardware/gpio.h>

static int gpio_drive(enum zbook_pin_header gpio, const struct zbook_gpio_ctx *ctx,
		      enum zbook_pin_header_state state)
{
	switch (state) {
	case ZBOOK_PIN_HEADER_STATE_LOW:
		gpio_set_dir(gpio, GPIO_OUT);
		gpio_disable_pulls(gpio);
		gpio_put(gpio, false);
		return 0;
	case ZBOOK_PIN_HEADER_STATE_HIGH:
		if (ctx->open_drain) {
			gpio_set_dir(gpio, GPIO_IN);
		} else {
			gpio_set_dir(gpio, GPIO_OUT);
			gpio_disable_pulls(gpio);
			gpio_put(gpio, true);
		}
		return 0;
	case ZBOOK_PIN_HEADER_STATE_OPEN_DRAIN:
		gpio_set_dir(gpio, GPIO_IN);
		gpio_disable_pulls(gpio);
		return 0;
	default:
		return -EINVAL;
	}
}

int zbook_io_gpio_init(enum zbook_pin_header gpio, const struct zbook_pin_cfg_gpio *cfg,
			    struct zbook_gpio_ctx *ctx)
{
	if (cfg == NULL || ctx == NULL) {
		return -EINVAL;
	}

	if (cfg->direction != ZBOOK_PIN_HEADER_DIRECTION_INPUT &&
	    cfg->direction != ZBOOK_PIN_HEADER_DIRECTION_OUTPUT) {
		return -EINVAL;
	}

	if (cfg->pull > ZBOOK_PIN_HEADER_PULL_DOWN) {
		return -EINVAL;
	}

	if (cfg->direction == ZBOOK_PIN_HEADER_DIRECTION_OUTPUT &&
	    cfg->state > ZBOOK_PIN_HEADER_STATE_OPEN_DRAIN) {
		return -EINVAL;
	}

	gpio_init(gpio);

	gpio_disable_pulls(gpio);

	if (cfg->pull == ZBOOK_PIN_HEADER_PULL_UP) {
		gpio_pull_up(gpio);
	} else if (cfg->pull == ZBOOK_PIN_HEADER_PULL_DOWN) {
		gpio_pull_down(gpio);
	}

	ctx->direction = cfg->direction;
	ctx->open_drain = (cfg->state == ZBOOK_PIN_HEADER_STATE_OPEN_DRAIN);

	if (cfg->direction == ZBOOK_PIN_HEADER_DIRECTION_OUTPUT) {
		return gpio_drive(gpio, ctx,
				  ctx->open_drain ? ZBOOK_PIN_HEADER_STATE_HIGH : cfg->state);
	}

	return 0;
}

void zbook_io_gpio_deinit(enum zbook_pin_header gpio)
{
	gpio_set_dir(gpio, GPIO_IN);
	gpio_disable_pulls(gpio);
	gpio_set_function(gpio, GPIO_FUNC_NULL);
}

int zbook_io_gpio_write(enum zbook_pin_header gpio, const struct zbook_gpio_ctx *ctx,
			     const void *buf, size_t len)
{
	if (len != 1) {
		return -EINVAL;
	}

	if (ctx->direction != ZBOOK_PIN_HEADER_DIRECTION_OUTPUT) {
		return -ENOTSUP;
	}

	return gpio_drive(gpio, ctx, *(const uint8_t *)buf);
}

int zbook_io_gpio_read(enum zbook_pin_header gpio, const struct zbook_gpio_ctx *ctx,
			    void *buf, size_t len)
{
	ARG_UNUSED(ctx);

	if (len != 1) {
		return -EINVAL;
	}

	*(uint8_t *)buf =
		gpio_get(gpio) ? ZBOOK_PIN_HEADER_STATE_HIGH : ZBOOK_PIN_HEADER_STATE_LOW;

	return 0;
}
