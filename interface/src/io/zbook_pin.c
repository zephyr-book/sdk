/*************************************************************************
 * @file zbook_pin.c
 *
 * @brief Implementation of the Zbook header pin-function factory.
 * @author João Matheus Nascimento Dias (joao.dias@edge.ufal.br)
 * @version 0.1
 * @date 08/10/2026
 *
 * @copyright Centro de Inovação EDGE 2026. Todos os direitos reservados.
 *
 ***********************************************************************/

#include "io/zbook_pin.h"

#include "io/zbook_io_adc.h"
#include "io/zbook_io_gpio.h"
#include "io/zbook_io_pwm.h"

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(zbook_pins, CONFIG_ZBOOK_INTERFACE_IO_PINS_LOG_LEVEL);

struct zbook_pin_backend {
	int (*init)(struct zbook_pin *pin, const struct zbook_pin_header_config *cfg);
	void (*deinit)(struct zbook_pin *pin);
	int (*read)(struct zbook_pin *pin, void *buf, size_t len);
	int (*write)(struct zbook_pin *pin, const void *buf, size_t len);
};

/**
 * @brief Array of Zbook header pins.
 *
 */
static struct zbook_pin pins[ZBOOK_PIN_HEADER_AMOUNT] = {
	{.lock = Z_MUTEX_INITIALIZER(pins[0].lock), .gpio = ZBOOK_PIN_HEADER_IO_01}, /**< GPIO1  */
	{.lock = Z_MUTEX_INITIALIZER(pins[1].lock), .gpio = ZBOOK_PIN_HEADER_IO_39}, /**< GPIO39 */
	{.lock = Z_MUTEX_INITIALIZER(pins[2].lock), .gpio = ZBOOK_PIN_HEADER_IO_45}, /**< GPIO45 */
	{.lock = Z_MUTEX_INITIALIZER(pins[3].lock), .gpio = ZBOOK_PIN_HEADER_IO_46}, /**< GPIO46 */
};

/**
 * @brief Macro to create a bitmask for a given function.
 *
 */
#define CAP(_fn) BIT(_fn)

/**
 * @brief Macro to define the capabilities of a Zbook header pin.
 *
 */
#define CAPS_COMMON                                                                                \
	(CAP(ZBOOK_PIN_HEADER_FUNCTION_GPIO) | CAP(ZBOOK_PIN_HEADER_FUNCTION_UART) |               \
	 CAP(ZBOOK_PIN_HEADER_FUNCTION_PWM))

/**
 * @brief Macro to define the capabilities of a Zbook header pin with ADC support.
 *
 */
#define CAPS_ADC (CAPS_COMMON | CAP(ZBOOK_PIN_HEADER_FUNCTION_ADC))

static const uint8_t pin_caps[ZBOOK_PIN_HEADER_AMOUNT] = {
	CAPS_COMMON, /* GPIO1  */
	CAPS_COMMON, /* GPIO39 */
	CAPS_ADC,    /* GPIO45 */
	CAPS_ADC,    /* GPIO46 */
};

static struct zbook_pin *pin_lookup(enum zbook_pin_header gpio)
{
	for (size_t i = 0; i < ARRAY_SIZE(pins); i++) {
		if (pins[i].gpio == gpio) {
			return &pins[i];
		}
	}

	return NULL;
}

static inline bool is_valid_handle(const struct zbook_pin *pin)
{
	return pin >= pins && pin < pins + ARRAY_SIZE(pins);
}

static int gpio_backend_init(struct zbook_pin *pin, const struct zbook_pin_header_config *cfg)
{
	return zbook_io_gpio_init(pin->gpio, &cfg->gpio, &pin->ctx.gpio);
}

static void gpio_backend_deinit(struct zbook_pin *pin)
{
	zbook_io_gpio_deinit(pin->gpio);
}

static int gpio_backend_write(struct zbook_pin *pin, const void *buf, size_t len)
{
	return zbook_io_gpio_write(pin->gpio, &pin->ctx.gpio, buf, len);
}

static int gpio_backend_read(struct zbook_pin *pin, void *buf, size_t len)
{
	return zbook_io_gpio_read(pin->gpio, &pin->ctx.gpio, buf, len);
}

static int pwm_backend_init(struct zbook_pin *pin, const struct zbook_pin_header_config *cfg)
{
	return zbook_io_pwm_init(pin->gpio, &cfg->pwm, &pin->ctx.pwm);
}

static void pwm_backend_deinit(struct zbook_pin *pin)
{
	zbook_io_pwm_deinit(pin->gpio);
}

static int pwm_backend_write(struct zbook_pin *pin, const void *buf, size_t len)
{
	return zbook_io_pwm_write(pin->gpio, &pin->ctx.pwm, buf, len);
}

#ifdef CONFIG_ADC
static int adc_backend_init(struct zbook_pin *pin, const struct zbook_pin_header_config *cfg)
{
	return zbook_io_adc_init(pin->gpio, &cfg->adc, &pin->ctx.adc);
}

static void adc_backend_deinit(struct zbook_pin *pin)
{
	zbook_io_adc_deinit(pin->gpio);
}

static int adc_backend_read(struct zbook_pin *pin, void *buf, size_t len)
{
	return zbook_io_adc_read(pin->gpio, &pin->ctx.adc, buf, len);
}
#endif /* CONFIG_ADC */

static const struct zbook_pin_backend backends[] = {
	[ZBOOK_PIN_HEADER_FUNCTION_GPIO] = {gpio_backend_init, gpio_backend_deinit,
					    gpio_backend_read, gpio_backend_write},
	[ZBOOK_PIN_HEADER_FUNCTION_PWM] = {pwm_backend_init, pwm_backend_deinit, NULL,
					   pwm_backend_write},
#ifdef CONFIG_ADC
	[ZBOOK_PIN_HEADER_FUNCTION_ADC] = {adc_backend_init, adc_backend_deinit, adc_backend_read,
					   NULL},
#endif
};

static const struct zbook_pin_backend *backend_for(enum zbook_pin_header_function function)
{
	if ((size_t)function >= ARRAY_SIZE(backends) || backends[function].init == NULL ||
	    backends[function].deinit == NULL) {
		LOG_DBG("Function {%d} has no backend.", function);
		return NULL;
	}

	return &backends[function];
}

static inline void pin_teardown(struct zbook_pin *pin)
{
	if (!pin->configured) {
		return;
	}

	const struct zbook_pin_backend *old = backend_for(pin->function);

	if (old != NULL) {
		old->deinit(pin);
	}

	LOG_DBG("GPIO%u: function %d released", pin->gpio, pin->function);
	pin->configured = false;
}

bool zbook_pin_supports(enum zbook_pin_header gpio, enum zbook_pin_header_function function)
{
	bool supported = false;

	for (size_t i = 0; i < ARRAY_SIZE(pins); i++) {
		if (pins[i].gpio == gpio) {
			bool is_valid_function = (unsigned int)function < 8;
			bool has_capability = (pin_caps[i] & CAP(function)) != 0;
			supported = is_valid_function && has_capability;
			break;
		}
	}

	return supported;
}

int zbook_pin_set_function(enum zbook_pin_header gpio, const struct zbook_pin_header_config *cfg,
			   struct zbook_pin **handle)
{
	struct zbook_pin *pin = pin_lookup(gpio);

	if (pin == NULL || cfg == NULL) {
		return -EINVAL;
	}

	if (!zbook_pin_supports(gpio, cfg->function)) {
		LOG_ERR("GPIO[%u] can not do function %d", gpio, cfg->function);
		return -ENOTSUP;
	}

	const struct zbook_pin_backend *next = backend_for(cfg->function);

	if (next == NULL) {
		LOG_ERR("function %d has no backend", cfg->function);
		return -ENOTSUP;
	}

	k_mutex_lock(&pin->lock, K_FOREVER);

	pin_teardown(pin);

	int ret = next->init(pin, cfg);
	if (ret != 0) {
		LOG_ERR("GPIO%u: function %d init failed (%d), pin left unconfigured", gpio,
			cfg->function, ret);

		k_mutex_unlock(&pin->lock);
		return ret;
	}

	pin->function = cfg->function;
	pin->configured = true;

	if (handle != NULL) {
		*handle = pin;
	}

	k_mutex_unlock(&pin->lock);

	LOG_DBG("GPIO%u: function %d configured", gpio, cfg->function);

	return ret;
}

int zbook_pin_release(struct zbook_pin *pin)
{
	if (!is_valid_handle(pin)) {
		return -EINVAL;
	}

	k_mutex_lock(&pin->lock, K_FOREVER);
	pin_teardown(pin);
	k_mutex_unlock(&pin->lock);

	return 0;
}

int zbook_pin_get_function(enum zbook_pin_header gpio, enum zbook_pin_header_function *function)
{
	struct zbook_pin *pin = pin_lookup(gpio);

	if (pin == NULL || function == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&pin->lock, K_FOREVER);

	int ret = pin->configured ? 0 : -ENODATA;

	if (ret == 0) {
		*function = pin->function;
	} else {
		LOG_DBG("GPIO[%u]: no function configured", gpio);
	}

	k_mutex_unlock(&pin->lock);

	return ret;
}

#define ZBOOK_PIN_CALL_BACKEND(_backend, _op, ...)                                                 \
	((_backend)->_op != NULL ? (_backend)->_op(__VA_ARGS__) : -ENOTSUP)

static inline int zbook_rw_apply(struct zbook_pin *pin, const void *buf, size_t len, bool is_read)
{
	if (!is_valid_handle(pin) || buf == NULL) {
		return -EINVAL;
	}

	int ret;

	k_mutex_lock(&pin->lock, K_FOREVER);

	if (!pin->configured) {
		k_mutex_unlock(&pin->lock);
		return -EINVAL;
	}

	const struct zbook_pin_backend *curr_backend = backend_for(pin->function);

	ret = is_read ? ZBOOK_PIN_CALL_BACKEND(curr_backend, read, pin, (void *)buf, len)
		      : ZBOOK_PIN_CALL_BACKEND(curr_backend, write, pin, buf, len);

	k_mutex_unlock(&pin->lock);

	return ret;
}

int zbook_pin_write(struct zbook_pin *handle, const void *buf, size_t len)
{
	return zbook_rw_apply(handle, buf, len, false);
}

int zbook_pin_read(struct zbook_pin *pin, void *buf, size_t len)
{
	return zbook_rw_apply(pin, buf, len, true);
}
