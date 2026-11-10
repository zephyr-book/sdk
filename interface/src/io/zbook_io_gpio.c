/*******************************************************************
 * @file zbook_io_gpio.c
 *
 * @brief GPIO backend of the Zbook header pins (see zbook_io_gpio.h).
 * @author José Félix de Oliveira Neto (josefelix.neto@edge.ufal.br)
 * @version 0.2
 * @date 06/11/26
 *
 * @copyright Copyright (c) 2026
 *
 *******************************************************************/

#include "io/zbook_io_gpio.h"
#include "io/zbook_pin.h"

#include <errno.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(zbook_io_gpio, CONFIG_ZBOOK_INTERFACE_IO_GPIO_LOG_LEVEL);

#define ZBOOK_GPIO_SPEC(_label)                                                                    \
	COND_CODE_1(DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(_label)),                                 \
		    (GPIO_DT_SPEC_GET(DT_NODELABEL(_label), gpios)), ({0}))

#define ZBOOK_GPIO_CHANNEL(_gpio, _label)                                                          \
	{                                                                                          \
		.gpio = (_gpio),                                                                   \
		.spec = ZBOOK_GPIO_SPEC(_label),                                                   \
		.debounce = Z_WORK_DELAYABLE_INITIALIZER(debounce_handler),                        \
	}

/**
 * @brief GPIO channel structure.
 *
 */
struct zbook_gpio_channel {
	enum zbook_pin_header gpio;
	struct gpio_dt_spec spec;
	struct gpio_callback gpio_cb;
	struct k_work_delayable debounce;
	zbook_pin_gpio_cb_t callback;
	void *user_data;
	gpio_flags_t int_flags;
};

static void debounce_handler(struct k_work *work);

static struct zbook_gpio_channel channels[] = {
	ZBOOK_GPIO_CHANNEL(ZBOOK_PIN_HEADER_IO_01, gpio_io01),
	ZBOOK_GPIO_CHANNEL(ZBOOK_PIN_HEADER_IO_39, gpio_io39),
	ZBOOK_GPIO_CHANNEL(ZBOOK_PIN_HEADER_IO_45, gpio_io45),
	ZBOOK_GPIO_CHANNEL(ZBOOK_PIN_HEADER_IO_46, gpio_io46),
};

static struct k_spinlock irq_lock;

static const gpio_flags_t edge_flags[] = {
	[ZBOOK_PIN_HEADER_INTERRUPT_RISING] = GPIO_INT_EDGE_RISING,
	[ZBOOK_PIN_HEADER_INTERRUPT_FALLING] = GPIO_INT_EDGE_FALLING,
	[ZBOOK_PIN_HEADER_INTERRUPT_BOTH] = GPIO_INT_EDGE_BOTH,
};

static struct zbook_gpio_channel *channel_of(const struct zbook_pin *pin)
{
	for (size_t i = 0; i < ARRAY_SIZE(channels); i++) {
		if (channels[i].gpio == pin->gpio) {
			return channels[i].spec.port != NULL ? &channels[i] : NULL;
		}
	}

	return NULL;
}

static void debounce_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct zbook_gpio_channel *ch = CONTAINER_OF(dwork, struct zbook_gpio_channel, debounce);

	k_spinlock_key_t key = k_spin_lock(&irq_lock);
	zbook_pin_gpio_cb_t callback = ch->callback;
	void *user_data = ch->user_data;

	k_spin_unlock(&irq_lock, key);

	if (callback == NULL) {
		return;
	}

	callback(ch->gpio, user_data);

	key = k_spin_lock(&irq_lock);
	if (ch->callback != NULL) {
		gpio_pin_interrupt_configure_dt(&ch->spec, ch->int_flags);
	}
	k_spin_unlock(&irq_lock, key);
}

static void gpio_isr(const struct device *port, struct gpio_callback *cb, gpio_port_pins_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	struct zbook_gpio_channel *ch = CONTAINER_OF(cb, struct zbook_gpio_channel, gpio_cb);

	gpio_pin_interrupt_configure_dt(&ch->spec, GPIO_INT_DISABLE);
	k_work_reschedule(&ch->debounce, K_MSEC(CONFIG_ZBOOK_INTERFACE_IO_GPIO_DEBOUNCE_MS));
}

static void irq_detach(struct zbook_gpio_channel *ch)
{
	k_spinlock_key_t key = k_spin_lock(&irq_lock);
	bool attached = ch->callback != NULL;

	if (attached) {
		gpio_pin_interrupt_configure_dt(&ch->spec, GPIO_INT_DISABLE);
	}
	ch->callback = NULL;
	ch->user_data = NULL;
	k_spin_unlock(&irq_lock, key);

	if (!attached) {
		return;
	}

	k_work_cancel_delayable(&ch->debounce);
	gpio_remove_callback_dt(&ch->spec, &ch->gpio_cb);
}

static int irq_attach(struct zbook_gpio_channel *ch, const struct zbook_pin_cfg_gpio *gpio)
{
	gpio_init_callback(&ch->gpio_cb, gpio_isr, BIT(ch->spec.pin));

	int ret = gpio_add_callback_dt(&ch->spec, &ch->gpio_cb);

	if (ret < 0) {
		return ret;
	}

	k_spinlock_key_t key = k_spin_lock(&irq_lock);

	ch->callback = gpio->callback;
	ch->user_data = gpio->user_data;
	ch->int_flags = edge_flags[gpio->interrupt];
	k_spin_unlock(&irq_lock, key);

	ret = gpio_pin_interrupt_configure_dt(&ch->spec, ch->int_flags);
	if (ret < 0) {
		irq_detach(ch);
	}

	return ret;
}

static int validate(const struct zbook_pin_cfg_gpio *gpio)
{
	if (gpio->direction > ZBOOK_PIN_HEADER_DIRECTION_OUTPUT ||
	    gpio->pull > ZBOOK_PIN_HEADER_PULL_DOWN ||
	    gpio->state > ZBOOK_PIN_HEADER_STATE_OPEN_DRAIN ||
	    gpio->interrupt > ZBOOK_PIN_HEADER_INTERRUPT_BOTH) {
		return -EINVAL;
	}

	if (gpio->interrupt != ZBOOK_PIN_HEADER_INTERRUPT_NONE &&
	    (gpio->direction != ZBOOK_PIN_HEADER_DIRECTION_INPUT || gpio->callback == NULL)) {
		return -EINVAL;
	}

	return 0;
}

static gpio_flags_t flags_of(const struct zbook_pin_cfg_gpio *gpio)
{
	/* Note: keeps the input buffer on so read() returns the pad level. */
	gpio_flags_t flags = GPIO_INPUT;

	if (gpio->direction == ZBOOK_PIN_HEADER_DIRECTION_OUTPUT) {
		if (gpio->state == ZBOOK_PIN_HEADER_STATE_OPEN_DRAIN) {
			flags |= GPIO_OUTPUT_HIGH | GPIO_OPEN_DRAIN;
		} else if (gpio->state == ZBOOK_PIN_HEADER_STATE_HIGH) {
			flags |= GPIO_OUTPUT_HIGH;
		} else {
			flags |= GPIO_OUTPUT_LOW;
		}
	}

	if (gpio->pull == ZBOOK_PIN_HEADER_PULL_UP) {
		flags |= GPIO_PULL_UP;
	} else if (gpio->pull == ZBOOK_PIN_HEADER_PULL_DOWN) {
		flags |= GPIO_PULL_DOWN;
	}

	return flags;
}

int zbook_io_gpio_init(struct zbook_pin *pin, const struct zbook_pin_header_config *cfg)
{
	if (pin == NULL || cfg == NULL) {
		return -EINVAL;
	}

	const struct zbook_pin_cfg_gpio *gpio = &cfg->cfg.gpio;

	if (validate(gpio) != 0) {
		LOG_ERR("GPIO%u: invalid config", pin->gpio);
		return -EINVAL;
	}

	struct zbook_gpio_channel *ch = channel_of(pin);

	if (ch == NULL || !gpio_is_ready_dt(&ch->spec)) {
		LOG_ERR("GPIO%u: not wired up or device not ready", pin->gpio);
		return -ENODEV;
	}

	int ret = gpio_pin_configure_dt(&ch->spec, flags_of(gpio));

	if (ret < 0) {
		LOG_ERR("GPIO%u: configure failed (%d)", pin->gpio, ret);
		return ret;
	}

	if (gpio->interrupt != ZBOOK_PIN_HEADER_INTERRUPT_NONE) {
		ret = irq_attach(ch, gpio);
		if (ret < 0) {
			LOG_ERR("GPIO%u: interrupt setup failed (%d)", pin->gpio, ret);
			gpio_pin_configure_dt(&ch->spec, GPIO_INPUT);
			return ret;
		}
	}

	pin->ctx.gpio.direction = gpio->direction;
	pin->ctx.gpio.open_drain = gpio->direction == ZBOOK_PIN_HEADER_DIRECTION_OUTPUT &&
				   gpio->state == ZBOOK_PIN_HEADER_STATE_OPEN_DRAIN;

	return 0;
}

void zbook_io_gpio_deinit(struct zbook_pin *pin)
{
	if (pin == NULL) {
		return;
	}

	struct zbook_gpio_channel *ch = channel_of(pin);

	if (ch != NULL) {
		irq_detach(ch);
		gpio_pin_configure_dt(&ch->spec, GPIO_INPUT);
	}

	pin->ctx.gpio.direction = ZBOOK_PIN_HEADER_DIRECTION_INPUT;
	pin->ctx.gpio.open_drain = false;
}

int zbook_io_gpio_write(struct zbook_pin *pin, const void *buf, size_t len)
{
	if (len != 1) {
		return -EINVAL;
	}

	if (pin->ctx.gpio.direction != ZBOOK_PIN_HEADER_DIRECTION_OUTPUT) {
		return -ENOTSUP;
	}

	uint8_t state = *(const uint8_t *)buf;

	if (state > ZBOOK_PIN_HEADER_STATE_HIGH) {
		return -EINVAL;
	}

	struct zbook_gpio_channel *ch = channel_of(pin);

	if (ch == NULL) {
		return -ENODEV;
	}

	return gpio_pin_set_dt(&ch->spec, state == ZBOOK_PIN_HEADER_STATE_HIGH);
}

int zbook_io_gpio_read(struct zbook_pin *pin, void *buf, size_t len)
{
	if (len != 1) {
		return -EINVAL;
	}

	struct zbook_gpio_channel *ch = channel_of(pin);

	if (ch == NULL) {
		return -ENODEV;
	}

	int ret = gpio_pin_get_dt(&ch->spec);

	if (ret < 0) {
		return ret;
	}

	*(uint8_t *)buf = ret ? ZBOOK_PIN_HEADER_STATE_HIGH : ZBOOK_PIN_HEADER_STATE_LOW;

	return 0;
}
