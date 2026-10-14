/*************************************************************************
 * @file zbook_io_pwm.c
 *
 * @brief Implementation of the Zbook PWM backend.
 * @author João Matheus Nascimento Dias (joao.dias@edge.ufal.br)
 * @version 0.1
 * @date 13/10/2026
 *
 * @copyright Centro de Inovação EDGE 2026. Todos os direitos reservados.
 *
 ***********************************************************************/

#include "io/zbook_io_pwm.h"

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <hardware/gpio.h>
#include <hardware/pwm.h>

/* The slice clock divider is 8.4 fixed point: 1.0 .. 255.9375. */
#define PWM_DIV_FRAC_BITS 4
#define PWM_DIV_ONE       BIT(PWM_DIV_FRAC_BITS)
#define PWM_DIV_MAX       (256U * PWM_DIV_ONE - 1U)
#define PWM_COUNTER_MAX   65536ULL

static int pwm_compute_timing(uint32_t frequency, uint32_t *div16, uint16_t *wrap)
{
	uint64_t clk = sys_clock_hw_cycles_per_sec();

	if (frequency == 0) {
		return -EINVAL;
	}

	/* Smallest divider (16ths) for which the period fits the 16 bit counter. */
	uint64_t d = DIV_ROUND_UP(clk * PWM_DIV_ONE, (uint64_t)frequency * PWM_COUNTER_MAX);

	d = MAX(d, (uint64_t)PWM_DIV_ONE);
	if (d > PWM_DIV_MAX) {
		return -EINVAL;
	}

	uint64_t ticks = (clk * PWM_DIV_ONE) / (d * frequency);

	if (ticks < 2) {
		return -EINVAL;
	}

	*div16 = (uint32_t)d;
	*wrap = (uint16_t)(MIN(ticks, PWM_COUNTER_MAX) - 1);

	return 0;
}

static void pwm_apply_duty(enum zbook_pin_header gpio, const struct zbook_pwm_ctx *ctx,
			   uint8_t duty)
{
	uint32_t period = (uint32_t)ctx->wrap + 1;
	uint32_t level = (period * duty) / 100;

	if (ctx->polarity == ZBOOK_PIN_HEADER_PWM_POLARITY_INVERTED) {
		level = period - level;
	}

	/* A level of period (wrap + 1) keeps the output constantly high. */
	pwm_set_chan_level(pwm_gpio_to_slice_num(gpio), pwm_gpio_to_channel(gpio),
			   (uint16_t)MIN(level, (uint32_t)UINT16_MAX));
}

int zbook_io_pwm_init(enum zbook_pin_header gpio, const struct zbook_pin_cfg_pwm *cfg,
		      struct zbook_pwm_ctx *ctx)
{
	uint32_t div16;
	uint16_t wrap;
	int ret;

	if (cfg == NULL || ctx == NULL || cfg->duty_cycle > 100 ||
	    cfg->polarity > ZBOOK_PIN_HEADER_PWM_POLARITY_INVERTED) {
		return -EINVAL;
	}

	ret = pwm_compute_timing(cfg->frequency, &div16, &wrap);
	if (ret != 0) {
		return ret;
	}

	uint slice = pwm_gpio_to_slice_num(gpio);

	ctx->polarity = cfg->polarity;
	ctx->wrap = wrap;

	pwm_set_enabled(slice, false);
	pwm_set_clkdiv_int_frac4(slice, div16 >> PWM_DIV_FRAC_BITS, div16 & (PWM_DIV_ONE - 1));
	pwm_set_wrap(slice, wrap);
	pwm_apply_duty(gpio, ctx, cfg->duty_cycle);

	gpio_set_function(gpio, GPIO_FUNC_PWM);
	pwm_set_enabled(slice, true);

	return 0;
}

void zbook_io_pwm_deinit(enum zbook_pin_header gpio)
{
	/* The slice keeps running: its other channel may belong to another pin. */
	pwm_set_chan_level(pwm_gpio_to_slice_num(gpio), pwm_gpio_to_channel(gpio), 0);

	gpio_set_dir(gpio, GPIO_IN);
	gpio_disable_pulls(gpio);
	gpio_set_function(gpio, GPIO_FUNC_NULL);
}

int zbook_io_pwm_write(enum zbook_pin_header gpio, const struct zbook_pwm_ctx *ctx, const void *buf,
		       size_t len)
{
	if (len != 1 || *(const uint8_t *)buf > 100) {
		return -EINVAL;
	}

	pwm_apply_duty(gpio, ctx, *(const uint8_t *)buf);

	return 0;
}
