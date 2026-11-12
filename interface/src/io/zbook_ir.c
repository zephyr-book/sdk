/************************************************************************
 * @file zbook_ir.c
 *
 * @brief Implements the interface for the Zbook infrared (IR) input/output.
 * @author Matheus Macário dos Santos (matheus.macario@edge.ufal.br)
 * @version 0.1
 * @date 10/11/2026
 *
 * @copyright Centro de Inovação EDGE 2026. Todos os direitos reservados.
 *
 ***********************************************************************/

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/util.h>

#include "io/zbook_ir.h"

LOG_MODULE_REGISTER(zbook_ir, CONFIG_ZBOOK_INTERFACE_IO_IR_LOG_LEVEL);

#define IR_EMITTER_NODE  DT_NODELABEL(ir_emitter)
#define IR_RECEIVER_NODE DT_NODELABEL(ir_receiver)

#define IS_IR_INITIALIZED()                                                                        \
	do {                                                                                       \
		if (!self.initialized) {                                                           \
			LOG_ERR("IR interface not initialized");                                   \
			return -ENODEV;                                                            \
		}                                                                                  \
	} while (0)

#define IS_IR_EVT_VALID(_evt)                                                                      \
	do {                                                                                       \
		if ((_evt) == ZBOOK_IR_EVT_NONE) {                                                 \
			LOG_ERR("Empty IR event mask");                                            \
			return -EINVAL;                                                            \
		}                                                                                  \
		if (((_evt) & ~ZBOOK_IR_EVT_BOTH) != 0) {                                          \
			LOG_ERR("Invalid IR event mask: %d", (_evt));                              \
			return -EINVAL;                                                            \
		}                                                                                  \
	} while (0)

#define IS_THREAD_CONTEXT()                                                                        \
	do {                                                                                       \
		if (k_is_in_isr()) {                                                               \
			LOG_ERR("Must be called from thread context");                             \
			return -EWOULDBLOCK;                                                       \
		}                                                                                  \
	} while (0)

static void zbook_ir_pulse_expiry(struct k_timer *timer);
static void zbook_ir_evt_work_handler(struct k_work *work);

static K_TIMER_DEFINE(pulse_timer, zbook_ir_pulse_expiry, NULL);
static K_WORK_DELAYABLE_DEFINE(evt_work, zbook_ir_evt_work_handler);

static K_MUTEX_DEFINE(cb_lock);

static struct {
	const struct gpio_dt_spec emitter;  /**< The IR emitter GPIO */
	const struct gpio_dt_spec receiver; /**< The IR receiver GPIO */
	struct gpio_callback gpio_cb;       /**< The receiver edge interrupt callback */
	struct k_spinlock lock;             /**< Guards the emitter state */
	bool emitter_on;                    /**< The logical state of the emitter */
	int receiver_level;                 /**< The last receiver level reported as event */
	bool has_reference;                 /**< Whether receiver_level holds a valid level */
	enum zbook_ir_evt evt_mask;         /**< The event mask the callback is registered for */
	zbook_ir_cb_t cb;                   /**< The callback function for the receiver events */
	void *user_data;                    /**< User-defined data for the callback */
	bool initialized;                   /**< Whether zbook_ir_init() has succeeded */
} self = {
	.emitter = GPIO_DT_SPEC_GET(IR_EMITTER_NODE, gpios),
	.receiver = GPIO_DT_SPEC_GET(IR_RECEIVER_NODE, gpios),
	.evt_mask = ZBOOK_IR_EVT_NONE,
};

static int zbook_ir_emitter_set_locked(bool on)
{
	int ret = gpio_pin_set_dt(&self.emitter, on);

	if (ret < 0) {
		LOG_ERR("Failed to drive IR emitter (%d)", ret);
		return ret;
	}

	self.emitter_on = on;

	return 0;
}

static int zbook_ir_emitter_set(bool on)
{
	k_spinlock_key_t key = k_spin_lock(&self.lock);

	k_timer_stop(&pulse_timer);

	int ret = zbook_ir_emitter_set_locked(on);

	k_spin_unlock(&self.lock, key);

	return ret;
}

static void zbook_ir_pulse_expiry(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	k_spinlock_key_t key = k_spin_lock(&self.lock);

	zbook_ir_emitter_set_locked(false);

	k_spin_unlock(&self.lock, key);
}

static void zbook_ir_evt_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	k_mutex_lock(&cb_lock, K_FOREVER);

	int level = gpio_pin_get_dt(&self.receiver);

	if (level < 0) {
		LOG_ERR("Failed to read IR receiver (%d)", level);
		k_mutex_unlock(&cb_lock);
		return;
	}

	if (level == self.receiver_level) {
		k_mutex_unlock(&cb_lock);
		return;
	}

	self.receiver_level = level;

	LOG_DBG("IR receiver level %d", level);

	enum zbook_ir_evt evt = level ? ZBOOK_IR_EVT_DETECTED : ZBOOK_IR_EVT_LOST;

	if ((self.evt_mask & evt) != 0) {
		self.cb(evt, self.user_data);
	}

	k_mutex_unlock(&cb_lock);
}

static void zbook_ir_receiver_isr(const struct device *port, struct gpio_callback *cb,
				  gpio_port_pins_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	k_work_reschedule(&evt_work, K_MSEC(CONFIG_ZBOOK_INTERFACE_IO_IR_SETTLE_MS));
}

static int zbook_ir_init_locked(void)
{
	int ret;

	self.initialized = false;

	ret = gpio_pin_interrupt_configure_dt(&self.receiver, GPIO_INT_DISABLE);
	if (ret < 0) {
		LOG_ERR("Failed to disable IR receiver interrupt (%d)", ret);
		return ret;
	}

	k_timer_stop(&pulse_timer);

	ret = gpio_pin_configure_dt(&self.emitter, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure IR emitter (%d)", ret);
		return ret;
	}

	self.emitter_on = false;

	ret = gpio_pin_configure_dt(&self.receiver, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure IR receiver (%d)", ret);
		return ret;
	}

	gpio_init_callback(&self.gpio_cb, zbook_ir_receiver_isr, BIT(self.receiver.pin));

	ret = gpio_add_callback_dt(&self.receiver, &self.gpio_cb);
	if (ret < 0) {
		LOG_ERR("Failed to add IR receiver callback (%d)", ret);
		return ret;
	}

	if (!self.has_reference) {
		ret = gpio_pin_get_dt(&self.receiver);
		if (ret < 0) {
			LOG_ERR("Failed to read IR receiver (%d)", ret);
			return ret;
		}

		self.receiver_level = ret;
		self.has_reference = true;
	}

	ret = gpio_pin_interrupt_configure_dt(&self.receiver, GPIO_INT_EDGE_BOTH);
	if (ret < 0) {
		LOG_ERR("Failed to enable IR receiver interrupt (%d)", ret);
		return ret;
	}

	k_work_reschedule(&evt_work, K_MSEC(CONFIG_ZBOOK_INTERFACE_IO_IR_SETTLE_MS));

	self.initialized = true;

	return 0;
}

int zbook_ir_init(void)
{
	if (!gpio_is_ready_dt(&self.emitter)) {
		LOG_ERR("IR emitter device not ready");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&self.receiver)) {
		LOG_ERR("IR receiver device not ready");
		return -ENODEV;
	}

	k_mutex_lock(&cb_lock, K_FOREVER);

	int ret = zbook_ir_init_locked();

	k_mutex_unlock(&cb_lock);

	return ret;
}

int zbook_ir_emitter_on(void)
{
	LOG_DBG("Turning on IR emitter");

	IS_IR_INITIALIZED();

	return zbook_ir_emitter_set(true);
}

int zbook_ir_emitter_off(void)
{
	LOG_DBG("Turning off IR emitter");

	IS_IR_INITIALIZED();

	return zbook_ir_emitter_set(false);
}

int zbook_ir_emitter_toggle(void)
{
	LOG_DBG("Toggling IR emitter");

	IS_IR_INITIALIZED();

	k_spinlock_key_t key = k_spin_lock(&self.lock);

	k_timer_stop(&pulse_timer);

	int ret = zbook_ir_emitter_set_locked(!self.emitter_on);

	k_spin_unlock(&self.lock, key);

	return ret;
}

int zbook_ir_emitter_get(void)
{
	IS_IR_INITIALIZED();

	return self.emitter_on ? 1 : 0;
}

int zbook_ir_emitter_pulse(uint32_t duration_ms)
{
	LOG_DBG("Pulsing IR emitter for %u ms", duration_ms);

	if (duration_ms == 0) {
		LOG_ERR("Invalid IR pulse duration: %u", duration_ms);
		return -EINVAL;
	}

	if (duration_ms > CONFIG_ZBOOK_INTERFACE_IO_IR_PULSE_MAX_MS) {
		LOG_ERR("Invalid IR pulse duration: %u", duration_ms);
		return -EINVAL;
	}

	IS_IR_INITIALIZED();

	k_spinlock_key_t key = k_spin_lock(&self.lock);
	int ret = zbook_ir_emitter_set_locked(true);

	if (ret == 0) {
		k_timer_start(&pulse_timer, K_MSEC(duration_ms), K_NO_WAIT);
	}

	k_spin_unlock(&self.lock, key);

	return ret;
}

int zbook_ir_receiver_read(void)
{
	IS_IR_INITIALIZED();

	return gpio_pin_get_dt(&self.receiver);
}

int zbook_ir_reg_cb(enum zbook_ir_evt evt, zbook_ir_cb_t cb, void *user_data)
{
	IS_THREAD_CONTEXT();
	IS_IR_EVT_VALID(evt);

	if (cb == NULL) {
		LOG_ERR("Callback must not be NULL");

		return -EINVAL;
	}

	k_mutex_lock(&cb_lock, K_FOREVER);

	self.evt_mask = evt;
	self.cb = cb;
	self.user_data = user_data;

	k_mutex_unlock(&cb_lock);

	return 0;
}

int zbook_ir_rm_cb(void)
{
	IS_THREAD_CONTEXT();

	k_mutex_lock(&cb_lock, K_FOREVER);

	self.evt_mask = ZBOOK_IR_EVT_NONE;
	self.cb = NULL;
	self.user_data = NULL;

	k_mutex_unlock(&cb_lock);

	return 0;
}
