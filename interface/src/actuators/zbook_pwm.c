/*******************************************************************
 * @file zbook_pwm.c
 *
 * @brief Implements the interface for zbook generic PWM output channels.
 * @author José Félix de Oliveira Neto (josefelix.neto@edge.ufal.br)
 * @version 0.1
 * @date 16/09/26
 *
 * @copyright Copyright (c) 2026
 *
 *******************************************************************/

#include "actuators/zbook_pwm.h"

#ifdef CONFIG_ZBOOK_PWM

#include <errno.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/clock.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(zbook_pwm, CONFIG_ZBOOK_PWM_LOG_LEVEL);

#define ZBOOK_PWM_CHANNEL_ENTRY(node_id)                                                           \
	{                                                                                          \
		.pwm = PWM_DT_SPEC_GET(node_id),                                                   \
	},

struct zbook_pwm_channel {
	struct pwm_dt_spec pwm;
	bool ready;
	bool playing;
	struct zbook_pwm_cfg cfg;
};

#define ZBOOK_PWM_CTLR_NODE DT_NODELABEL(pwm)

#if DT_NODE_EXISTS(ZBOOK_PWM_CTLR_NODE) && DT_NODE_HAS_PROP(ZBOOK_PWM_CTLR_NODE, pinctrl_1)

#define ZBOOK_PWM_HAS_ZBOOK_IO_STATE 1

#include <zephyr/drivers/pinctrl.h>

PINCTRL_DT_DEV_CONFIG_DECLARE(ZBOOK_PWM_CTLR_NODE);
static struct pinctrl_dev_config *zbook_pwm_pcfg = PINCTRL_DT_DEV_CONFIG_GET(ZBOOK_PWM_CTLR_NODE);

#endif

#if DT_NODE_EXISTS(DT_NODELABEL(zbook_pwm_channels))

static struct zbook_pwm_channel channels[] = {
	DT_FOREACH_CHILD_STATUS_OKAY(DT_NODELABEL(zbook_pwm_channels), ZBOOK_PWM_CHANNEL_ENTRY)};

#else

static struct zbook_pwm_channel channels[] = {};

#endif

static struct zbook_pwm_channel *find_channel(enum zbook_pwm_io io)
{
	if ((size_t)io >= ARRAY_SIZE(channels)) {
		return NULL;
	}

	return &channels[io];
}

static int claim_pin(const struct zbook_pwm_channel *ch)
{
#if defined(ZBOOK_PWM_HAS_ZBOOK_IO_STATE)
	const struct pinctrl_state *state;
	size_t idx = ch - channels;
	int ret;

	ret = pinctrl_lookup_state(zbook_pwm_pcfg, PINCTRL_STATE_ZBOOK_IO, &state);
	if (ret < 0) {
		return ret;
	}

	if (idx >= state->pin_cnt) {
		return -ERANGE;
	}

#ifdef CONFIG_PINCTRL_STORE_REG
	uintptr_t reg = zbook_pwm_pcfg->reg;
#else
	uintptr_t reg = PINCTRL_REG_NONE;
#endif

	return pinctrl_configure_pins(&state->pins[idx], 1, reg);
#else
	ARG_UNUSED(ch);

	return 0;
#endif
}

static int apply(const struct zbook_pwm_channel *ch)
{
	uint32_t period_ns = NSEC_PER_SEC / ch->cfg.frequency_hz;
	uint32_t pulse_ns =
		ch->playing ? ((uint64_t)period_ns * ch->cfg.duty_cycle_percent) / 100U : 0U;
	pwm_flags_t flags = ch->cfg.polarity == ZBOOK_PWM_POLARITY_INVERTED ? PWM_POLARITY_INVERTED
									    : PWM_POLARITY_NORMAL;

	return pwm_set(ch->pwm.dev, ch->pwm.channel, period_ns, pulse_ns, flags);
}

int zbook_pwm_init(enum zbook_pwm_io io)
{
	struct zbook_pwm_channel *ch = find_channel(io);

	if (ch == NULL) {
		LOG_ERR("zbook_pwm io %d not wired up in this build", io);
		return -ENODEV;
	}

	int ret = claim_pin(ch);

	if (ret < 0) {
		LOG_ERR("zbook_pwm io %d pin claim failed (%d)", io, ret);
		return ret;
	}

	if (!pwm_is_ready_dt(&ch->pwm)) {
		LOG_ERR("zbook_pwm io %d device not ready", io);
		return -ENODEV;
	}

	ch->ready = true;
	ch->playing = false;
	ch->cfg = (struct zbook_pwm_cfg){
		.frequency_hz = NSEC_PER_SEC / ch->pwm.period,
		.duty_cycle_percent = 0,
		.polarity = ZBOOK_PWM_POLARITY_NORMAL,
	};

	LOG_DBG("zbook_pwm io %d initialized", io);

	return apply(ch);
}

int zbook_pwm_start(enum zbook_pwm_io io)
{
	struct zbook_pwm_channel *ch = find_channel(io);

	if (ch == NULL || !ch->ready) {
		LOG_ERR("zbook_pwm io %d not initialized", io);
		return -ENODEV;
	}

	ch->playing = true;

	return apply(ch);
}

int zbook_pwm_stop(enum zbook_pwm_io io)
{
	struct zbook_pwm_channel *ch = find_channel(io);

	if (ch == NULL || !ch->ready) {
		LOG_ERR("zbook_pwm io %d not initialized", io);
		return -ENODEV;
	}

	ch->playing = false;

	return apply(ch);
}

int zbook_pwm_set_cfg(enum zbook_pwm_io io, const struct zbook_pwm_cfg *cfg)
{
	if (cfg == NULL || cfg->frequency_hz == 0 || cfg->duty_cycle_percent > 100) {
		LOG_ERR("invalid zbook_pwm config for io %d", io);
		return -EINVAL;
	}

	struct zbook_pwm_channel *ch = find_channel(io);

	if (ch == NULL || !ch->ready) {
		LOG_ERR("zbook_pwm io %d not initialized", io);
		return -ENODEV;
	}

	ch->cfg = *cfg;

	return apply(ch);
}

int zbook_pwm_get_cfg(enum zbook_pwm_io io, struct zbook_pwm_cfg *cfg)
{
	if (cfg == NULL) {
		LOG_ERR("NULL zbook_pwm cfg output for io %d", io);
		return -EINVAL;
	}

	struct zbook_pwm_channel *ch = find_channel(io);

	if (ch == NULL || !ch->ready) {
		LOG_ERR("zbook_pwm io %d not initialized", io);
		return -ENODEV;
	}

	*cfg = ch->cfg;

	return 0;
}

#endif /* CONFIG_ZBOOK_PWM */
