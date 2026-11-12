/************************************************************************
 * @file main.c
 *
 * @brief Unit tests for the Zbook IR interface.
 * @author Matheus Macário dos Santos (matheus.macario@edge.ufal.br)
 * @version 0.1
 * @date 10/11/2026
 *
 * @copyright Centro de Inovação EDGE 2026. Todos os direitos reservados.
 *
 ***********************************************************************/

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/sys/atomic.h>

#include "io/zbook_ir.h"

#define SETTLE_MS       CONFIG_ZBOOK_INTERFACE_IO_IR_SETTLE_MS
#define PULSE_MAX_MS    CONFIG_ZBOOK_INTERFACE_IO_IR_PULSE_MAX_MS
#define WAIT_TIMEOUT    K_MSEC(100)
#define NO_EVT_TIMEOUT  K_MSEC(MAX(SETTLE_MS * 4, 20))
#define SETTLE_WAIT_MS  (MAX(SETTLE_MS, 1) * 3 + 5)
#define PULSE_MS        20
#define PULSE_MARGIN_MS 10
#define SLOW_CB_MS      50
#define LOG_MAX         16

#define EMITTER_NODE  DT_NODELABEL(ir_emitter)
#define RECEIVER_NODE DT_NODELABEL(ir_receiver)

#define EMITTER_ACTIVE_LOW  ((DT_GPIO_FLAGS(EMITTER_NODE, gpios) & GPIO_ACTIVE_LOW) != 0)
#define RECEIVER_ACTIVE_LOW ((DT_GPIO_FLAGS(RECEIVER_NODE, gpios) & GPIO_ACTIVE_LOW) != 0)

static const struct gpio_dt_spec emitter = GPIO_DT_SPEC_GET(EMITTER_NODE, gpios);
static const struct gpio_dt_spec receiver = GPIO_DT_SPEC_GET(RECEIVER_NODE, gpios);

enum cb_action {
	ACT_NONE,
	ACT_RM,
	ACT_REG_LOST,
	ACT_INIT,
	ACT_PULSE,
};

static struct {
	struct k_sem sem;
	struct k_sem entered;
	enum zbook_ir_evt log[LOG_MAX];
	atomic_t count;
	bool in_isr;
	bool in_sys_workq;
	int sleep_ms;
	bool done;
	enum cb_action action;
	int action_ret;
} rec;

static int other_count;

static int emitter_phys(void)
{
	return gpio_emul_output_get(emitter.port, emitter.pin);
}

static int emitter_logical(void)
{
	return EMITTER_ACTIVE_LOW ? !emitter_phys() : emitter_phys();
}

static void rx_set(int logical)
{
	int phys = RECEIVER_ACTIVE_LOW ? !logical : logical;

	zassert_ok(gpio_emul_input_set(receiver.port, receiver.pin, phys));
}

static void rec_reset(void)
{
	k_sem_init(&rec.sem, 0, LOG_MAX);
	k_sem_init(&rec.entered, 0, LOG_MAX);
	memset(rec.log, 0, sizeof(rec.log));
	atomic_set(&rec.count, 0);
	rec.in_isr = true;
	rec.in_sys_workq = false;
	rec.sleep_ms = 0;
	rec.done = false;
	rec.action = ACT_NONE;
	rec.action_ret = 0;
}

static void rec_cb(enum zbook_ir_evt evt, void *user_data)
{
	ARG_UNUSED(user_data);

	int n = atomic_inc(&rec.count);

	if (n < LOG_MAX) {
		rec.log[n] = evt;
	}

	rec.in_isr = k_is_in_isr();
	rec.in_sys_workq = k_current_get() == k_work_queue_thread_get(&k_sys_work_q);
	rec.done = false;
	k_sem_give(&rec.entered);

	if (rec.sleep_ms > 0) {
		k_msleep(rec.sleep_ms);
	}

	switch (rec.action) {
	case ACT_RM:
		rec.action_ret = zbook_ir_rm_cb();
		break;
	case ACT_REG_LOST:
		rec.action_ret = zbook_ir_reg_cb(ZBOOK_IR_EVT_LOST, rec_cb, NULL);
		break;
	case ACT_INIT:
		rec.action_ret = zbook_ir_init();
		break;
	case ACT_PULSE:
		rec.action_ret = zbook_ir_emitter_pulse(PULSE_MS);
		break;
	default:
		break;
	}

	rec.action = ACT_NONE;
	rec.done = true;
	k_sem_give(&rec.sem);
}

static void other_cb(enum zbook_ir_evt evt, void *user_data)
{
	ARG_UNUSED(evt);
	ARG_UNUSED(user_data);

	other_count++;
}

static int rec_count(void)
{
	return (int)atomic_get(&rec.count);
}

static void skip_without_settle(void)
{
	if (SETTLE_MS == 0) {
		ztest_test_skip();
	}
}

ZTEST_SUITE(zbook_hw_errors, NULL, NULL, NULL, NULL, NULL);

ZTEST(zbook_hw_errors, test_0_calls_before_init_return_enodev)
{
	zassert_equal(-ENODEV, zbook_ir_emitter_on());
	zassert_equal(-ENODEV, zbook_ir_emitter_off());
	zassert_equal(-ENODEV, zbook_ir_emitter_toggle());
	zassert_equal(-ENODEV, zbook_ir_emitter_get());
	zassert_equal(-ENODEV, zbook_ir_emitter_pulse(PULSE_MS));
	zassert_equal(-ENODEV, zbook_ir_receiver_read());

	zassert_equal(-EINVAL, zbook_ir_emitter_pulse(0));
	zassert_equal(-EINVAL, zbook_ir_emitter_pulse(PULSE_MAX_MS + 1));
}

ZTEST(zbook_hw_errors, test_1_init_fails_while_devices_not_ready)
{
	const struct device *tx_ctrl = DEVICE_DT_GET(DT_NODELABEL(gpio_emul_ir_tx));
	const struct device *rx_ctrl = DEVICE_DT_GET(DT_NODELABEL(gpio_emul_ir_rx));

	zassert_false(device_is_ready(tx_ctrl), "test setup: emitter controller already ready");
	zassert_false(device_is_ready(rx_ctrl), "test setup: receiver controller already ready");

	zassert_equal(-ENODEV, zbook_ir_init(), "emitter not ready");

	zassert_ok(device_init(tx_ctrl));
	zassert_equal(-ENODEV, zbook_ir_init(), "receiver not ready");
	zassert_equal(-ENODEV, zbook_ir_emitter_on());

	zassert_ok(device_init(rx_ctrl), "failed to bring up receiver controller for later tests");
	zassert_true(device_is_ready(rx_ctrl));
}

ZTEST(zbook_hw_errors, test_2_first_init_takes_level_as_reference)
{
	rec_reset();

	zassert_ok(zbook_ir_rm_cb(), "rm_cb before init");
	zassert_ok(zbook_ir_reg_cb(ZBOOK_IR_EVT_BOTH, rec_cb, NULL), "reg_cb before init");

	zassert_ok(gpio_pin_configure_dt(&receiver, GPIO_INPUT));
	rx_set(1);

	zassert_ok(zbook_ir_init());
	zassert_equal(0, emitter_logical(), "init must leave the emitter off");
	zassert_equal(1, zbook_ir_receiver_read());

	zassert_not_ok(k_sem_take(&rec.sem, NO_EVT_TIMEOUT), "event fired for the initial level");

	rx_set(0);
	zassert_ok(k_sem_take(&rec.sem, WAIT_TIMEOUT), "callback registered before init not fired");
	zassert_equal(ZBOOK_IR_EVT_LOST, rec.log[0]);
	zassert_equal(1, rec_count());

	zassert_ok(zbook_ir_rm_cb());
}

static void zbook_ir_before(void *fixture)
{
	ARG_UNUSED(fixture);

	zassert_ok(zbook_ir_init(), "zbook_ir_init failed");
	zassert_ok(zbook_ir_rm_cb());
	rx_set(0);

	k_msleep(SETTLE_WAIT_MS);

	rec_reset();
	other_count = 0;
}

ZTEST_SUITE(zbook_ir, NULL, NULL, zbook_ir_before, NULL, NULL);

ZTEST(zbook_ir, test_init_returns_ok_and_emitter_off)
{
	zassert_ok(zbook_ir_init());
	zassert_equal(0, zbook_ir_emitter_get());
	zassert_equal(0, emitter_logical());
}

ZTEST(zbook_ir, test_physical_level_follows_devicetree_polarity)
{
	int on_level = EMITTER_ACTIVE_LOW ? 0 : 1;

	zassert_equal(!on_level, emitter_phys(), "physical level wrong after init");

	zassert_ok(zbook_ir_emitter_on());
	zassert_equal(on_level, emitter_phys(), "on(): physical level wrong");

	zassert_ok(zbook_ir_emitter_off());
	zassert_equal(!on_level, emitter_phys(), "off(): physical level wrong");

	zassert_ok(zbook_ir_emitter_pulse(PULSE_MS));
	zassert_equal(on_level, emitter_phys(), "pulse(): physical level wrong");

	k_msleep(PULSE_MS + PULSE_MARGIN_MS);
	zassert_equal(!on_level, emitter_phys(), "pulse expiry: physical level wrong");
}

ZTEST(zbook_ir, test_on_turns_emitter_on)
{
	zassert_ok(zbook_ir_emitter_on());
	zassert_equal(1, emitter_logical());
	zassert_equal(1, zbook_ir_emitter_get());
}

ZTEST(zbook_ir, test_off_turns_emitter_off)
{
	zassert_ok(zbook_ir_emitter_on());
	zassert_ok(zbook_ir_emitter_off());
	zassert_equal(0, emitter_logical());
	zassert_equal(0, zbook_ir_emitter_get());
}

ZTEST(zbook_ir, test_toggle_flips_emitter)
{
	zassert_ok(zbook_ir_emitter_toggle());
	zassert_equal(1, emitter_logical());
	zassert_equal(1, zbook_ir_emitter_get());

	zassert_ok(zbook_ir_emitter_toggle());
	zassert_equal(0, emitter_logical());
	zassert_equal(0, zbook_ir_emitter_get());
}

ZTEST(zbook_ir, test_pulse_turns_off_after_duration)
{
	zassert_ok(zbook_ir_emitter_pulse(PULSE_MS));
	zassert_equal(1, emitter_logical());
	zassert_equal(1, zbook_ir_emitter_get());

	k_msleep(PULSE_MS - PULSE_MARGIN_MS);
	zassert_equal(1, emitter_logical(), "emitter turned off before the pulse duration");

	k_msleep(PULSE_MARGIN_MS * 2);
	zassert_equal(0, emitter_logical(), "emitter still on after the pulse duration");
	zassert_equal(0, zbook_ir_emitter_get());
}

ZTEST(zbook_ir, test_pulse_accepts_max_duration)
{
	zassert_ok(zbook_ir_emitter_pulse(PULSE_MAX_MS));
	zassert_equal(1, emitter_logical());

	k_msleep(PULSE_MAX_MS + PULSE_MARGIN_MS);
	zassert_equal(0, emitter_logical());
}

ZTEST(zbook_ir, test_pulse_restarts_with_longer_duration)
{
	zassert_ok(zbook_ir_emitter_pulse(PULSE_MS));
	k_msleep(PULSE_MS / 2);

	zassert_ok(zbook_ir_emitter_pulse(PULSE_MS * 3));

	k_msleep(PULSE_MS * 2);
	zassert_equal(1, emitter_logical(), "first pulse duration still applied");

	k_msleep(PULSE_MS + PULSE_MARGIN_MS);
	zassert_equal(0, emitter_logical(), "emitter still on after the second pulse");
}

ZTEST(zbook_ir, test_pulse_restarts_with_shorter_duration)
{
	zassert_ok(zbook_ir_emitter_pulse(PULSE_MS * 6));
	k_msleep(PULSE_MARGIN_MS / 2);

	zassert_ok(zbook_ir_emitter_pulse(PULSE_MARGIN_MS));

	k_msleep(PULSE_MARGIN_MS * 2);
	zassert_equal(0, emitter_logical(), "shorter restart did not shorten the pulse");
	zassert_equal(0, zbook_ir_emitter_get());
}

ZTEST(zbook_ir, test_pulse_while_on_turns_off_at_expiry)
{
	zassert_ok(zbook_ir_emitter_on());
	zassert_ok(zbook_ir_emitter_pulse(PULSE_MS));

	k_msleep(PULSE_MS + PULSE_MARGIN_MS);
	zassert_equal(0, emitter_logical(), "pulse over a steady on did not end off");
}

ZTEST(zbook_ir, test_on_cancels_pulse)
{
	zassert_ok(zbook_ir_emitter_pulse(PULSE_MS));
	zassert_ok(zbook_ir_emitter_on());

	k_msleep(PULSE_MS + PULSE_MARGIN_MS);
	zassert_equal(1, emitter_logical(), "pulse turned the emitter off after on()");
	zassert_equal(1, zbook_ir_emitter_get());
}

ZTEST(zbook_ir, test_off_cancels_pulse)
{
	zassert_ok(zbook_ir_emitter_pulse(PULSE_MS));
	zassert_ok(zbook_ir_emitter_off());
	zassert_equal(0, emitter_logical());

	zassert_ok(zbook_ir_emitter_on());
	k_msleep(PULSE_MS + PULSE_MARGIN_MS);
	zassert_equal(1, emitter_logical());
}

ZTEST(zbook_ir, test_toggle_cancels_pulse)
{
	zassert_ok(zbook_ir_emitter_pulse(PULSE_MS));
	k_msleep(PULSE_MARGIN_MS / 2);

	zassert_ok(zbook_ir_emitter_toggle());
	zassert_equal(0, emitter_logical(), "toggle of a running pulse must turn it off");

	k_msleep(PULSE_MARGIN_MS / 2);

	zassert_ok(zbook_ir_emitter_toggle());
	zassert_equal(1, emitter_logical());

	k_msleep(PULSE_MS + PULSE_MARGIN_MS);
	zassert_equal(1, emitter_logical(), "old pulse expiry fired after the toggles");
	zassert_equal(1, zbook_ir_emitter_get());
}

ZTEST(zbook_ir, test_pulse_rejects_invalid_duration)
{
	zassert_equal(-EINVAL, zbook_ir_emitter_pulse(0));
	zassert_equal(-EINVAL, zbook_ir_emitter_pulse(PULSE_MAX_MS + 1));
	zassert_equal(-EINVAL, zbook_ir_emitter_pulse(UINT32_MAX));
	zassert_equal(0, emitter_logical(), "rejected pulse changed the emitter");
}

ZTEST(zbook_ir, test_invalid_pulse_keeps_running_pulse)
{
	zassert_ok(zbook_ir_emitter_pulse(PULSE_MS));
	zassert_equal(-EINVAL, zbook_ir_emitter_pulse(0));
	zassert_equal(1, emitter_logical());

	k_msleep(PULSE_MS + PULSE_MARGIN_MS);
	zassert_equal(0, emitter_logical(), "rejected pulse cancelled the running one");
}

static uint32_t xorshift32(uint32_t *state)
{
	uint32_t x = *state;

	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	*state = x;

	return x;
}

ZTEST(zbook_ir, test_stress_cached_state_matches_pin)
{
	uint32_t seed = 0x1234abcd;
	int mismatches = 0;

	for (int i = 0; i < 500; i++) {
		uint32_t r = xorshift32(&seed);

		switch (r % 4) {
		case 0:
			zassert_ok(zbook_ir_emitter_on());
			break;
		case 1:
			zassert_ok(zbook_ir_emitter_off());
			break;
		case 2:
			zassert_ok(zbook_ir_emitter_toggle());
			break;
		default:
			zassert_ok(zbook_ir_emitter_pulse(1 + (r >> 8) % 3));
			break;
		}

		unsigned int key = irq_lock();
		int cached = zbook_ir_emitter_get();
		int pin = emitter_logical();

		irq_unlock(key);

		if (cached != pin) {
			mismatches++;
		}

		k_usleep((r >> 16) % 2500);
	}

	zassert_equal(0, mismatches, "cached emitter state diverged from the pin %d times",
		      mismatches);
}

ZTEST(zbook_ir, test_receiver_read_follows_level)
{
	rx_set(1);
	zassert_equal(1, zbook_ir_receiver_read());

	rx_set(0);
	zassert_equal(0, zbook_ir_receiver_read());
}

ZTEST(zbook_ir, test_receiver_read_ignores_settle_time)
{
	skip_without_settle();

	zassert_ok(zbook_ir_reg_cb(ZBOOK_IR_EVT_BOTH, rec_cb, NULL));

	rx_set(1);
	zassert_equal(1, zbook_ir_receiver_read());
	rx_set(0);
	zassert_equal(0, zbook_ir_receiver_read());

	zassert_not_ok(k_sem_take(&rec.sem, NO_EVT_TIMEOUT));
}

ZTEST(zbook_ir, test_reg_cb_rejects_invalid_event)
{
	zassert_equal(-EINVAL, zbook_ir_reg_cb(ZBOOK_IR_EVT_NONE, rec_cb, NULL));
	zassert_equal(-EINVAL, zbook_ir_reg_cb((enum zbook_ir_evt)BIT(2), rec_cb, NULL));
	zassert_equal(-EINVAL, zbook_ir_reg_cb((enum zbook_ir_evt)0x5A5A, rec_cb, NULL));
	zassert_equal(-EINVAL, zbook_ir_reg_cb((enum zbook_ir_evt) - 1, rec_cb, NULL));
}

ZTEST(zbook_ir, test_reg_cb_rejects_null_callback)
{
	zassert_equal(-EINVAL, zbook_ir_reg_cb(ZBOOK_IR_EVT_BOTH, NULL, NULL));
}

ZTEST(zbook_ir, test_invalid_reg_cb_keeps_previous_registration)
{
	zassert_ok(zbook_ir_reg_cb(ZBOOK_IR_EVT_BOTH, rec_cb, NULL));

	zassert_equal(-EINVAL, zbook_ir_reg_cb((enum zbook_ir_evt)BIT(3), other_cb, NULL));
	zassert_equal(-EINVAL, zbook_ir_reg_cb(ZBOOK_IR_EVT_DETECTED, NULL, NULL));

	rx_set(1);
	zassert_ok(k_sem_take(&rec.sem, WAIT_TIMEOUT), "registration clobbered by invalid one");
	zassert_equal(0, other_count);
}

ZTEST(zbook_ir, test_rm_cb_without_callback_succeeds)
{
	zassert_ok(zbook_ir_rm_cb());
}

static struct {
	struct k_timer timer;
	struct k_sem done;
	bool in_isr;
	int reg_ret;
	int rm_ret;
} isr_call;

static void isr_call_expiry(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	isr_call.in_isr = k_is_in_isr();
	isr_call.reg_ret = zbook_ir_reg_cb(ZBOOK_IR_EVT_BOTH, other_cb, NULL);
	isr_call.rm_ret = zbook_ir_rm_cb();
	k_sem_give(&isr_call.done);
}

ZTEST(zbook_ir, test_reg_rm_cb_from_isr_are_rejected)
{
	zassert_ok(zbook_ir_reg_cb(ZBOOK_IR_EVT_BOTH, rec_cb, NULL));

	k_sem_init(&isr_call.done, 0, 1);
	k_timer_init(&isr_call.timer, isr_call_expiry, NULL);
	k_timer_start(&isr_call.timer, K_MSEC(1), K_NO_WAIT);
	zassert_ok(k_sem_take(&isr_call.done, WAIT_TIMEOUT));

	zassert_true(isr_call.in_isr, "test setup: timer expiry not in interrupt context");
	zassert_equal(-EWOULDBLOCK, isr_call.reg_ret);
	zassert_equal(-EWOULDBLOCK, isr_call.rm_ret);

	rx_set(1);
	zassert_ok(k_sem_take(&rec.sem, WAIT_TIMEOUT));
	zassert_equal(0, other_count);
}

ZTEST(zbook_ir, test_detected_fires_only_on_detect)
{
	zassert_ok(zbook_ir_reg_cb(ZBOOK_IR_EVT_DETECTED, rec_cb, NULL));

	rx_set(1);
	zassert_ok(k_sem_take(&rec.sem, WAIT_TIMEOUT), "callback not fired on detect");
	zassert_equal(ZBOOK_IR_EVT_DETECTED, rec.log[0]);
	zassert_equal(1, rec_count());

	rx_set(0);
	zassert_not_ok(k_sem_take(&rec.sem, NO_EVT_TIMEOUT), "callback fired on lost");
	zassert_equal(1, rec_count());
}

ZTEST(zbook_ir, test_lost_fires_only_on_lost)
{
	zassert_ok(zbook_ir_reg_cb(ZBOOK_IR_EVT_LOST, rec_cb, NULL));

	rx_set(1);
	zassert_not_ok(k_sem_take(&rec.sem, NO_EVT_TIMEOUT), "callback fired on detect");
	zassert_equal(0, rec_count());

	rx_set(0);
	zassert_ok(k_sem_take(&rec.sem, WAIT_TIMEOUT), "callback not fired on lost");
	zassert_equal(ZBOOK_IR_EVT_LOST, rec.log[0]);
	zassert_equal(1, rec_count());
}

ZTEST(zbook_ir, test_both_fires_on_detect_and_lost)
{
	zassert_ok(zbook_ir_reg_cb(ZBOOK_IR_EVT_BOTH, rec_cb, NULL));

	rx_set(1);
	zassert_ok(k_sem_take(&rec.sem, WAIT_TIMEOUT), "callback not fired on detect");
	zassert_equal(ZBOOK_IR_EVT_DETECTED, rec.log[0]);

	rx_set(0);
	zassert_ok(k_sem_take(&rec.sem, WAIT_TIMEOUT), "callback not fired on lost");
	zassert_equal(ZBOOK_IR_EVT_LOST, rec.log[1]);

	zassert_equal(2, rec_count());
}

static void assert_mask_over_cycles(enum zbook_ir_evt mask)
{
	zassert_ok(zbook_ir_reg_cb(mask, rec_cb, NULL));

	for (int i = 0; i < 4; i++) {
		rx_set((i & 1) == 0);
		k_msleep(SETTLE_WAIT_MS);
	}

	zassert_equal(2, rec_count());
	zassert_equal(mask, rec.log[0]);
	zassert_equal(mask, rec.log[1]);
}

ZTEST(zbook_ir, test_mask_detected_over_cycles)
{
	assert_mask_over_cycles(ZBOOK_IR_EVT_DETECTED);
}

ZTEST(zbook_ir, test_mask_lost_over_cycles)
{
	assert_mask_over_cycles(ZBOOK_IR_EVT_LOST);
}

ZTEST(zbook_ir, test_callback_runs_in_thread_context)
{
	zassert_ok(zbook_ir_reg_cb(ZBOOK_IR_EVT_BOTH, rec_cb, NULL));

	rx_set(1);
	zassert_ok(k_sem_take(&rec.sem, WAIT_TIMEOUT));

	zassert_false(rec.in_isr, "callback ran in interrupt context");
	zassert_true(rec.in_sys_workq, "callback did not run in the system workqueue");
}

ZTEST(zbook_ir, test_event_waits_for_settle_time)
{
	zassert_ok(zbook_ir_reg_cb(ZBOOK_IR_EVT_BOTH, rec_cb, NULL));

	int64_t start = k_uptime_get();

	rx_set(1);
	zassert_ok(k_sem_take(&rec.sem, WAIT_TIMEOUT));

	zassert_true(k_uptime_get() - start >= SETTLE_MS, "event reported before the settle time");
}

ZTEST(zbook_ir, test_glitch_shorter_than_settle_is_filtered)
{
	skip_without_settle();

	zassert_ok(zbook_ir_reg_cb(ZBOOK_IR_EVT_BOTH, rec_cb, NULL));

	for (int i = 0; i < 10; i++) {
		rx_set((i & 1) == 0);
		k_busy_wait(300);
	}

	zassert_not_ok(k_sem_take(&rec.sem, NO_EVT_TIMEOUT), "glitch reported as event");
	zassert_equal(0, rec_count());
}

ZTEST(zbook_ir, test_dropout_after_detected_is_filtered)
{
	skip_without_settle();

	zassert_ok(zbook_ir_reg_cb(ZBOOK_IR_EVT_BOTH, rec_cb, NULL));

	rx_set(1);
	zassert_ok(k_sem_take(&rec.sem, WAIT_TIMEOUT));

	rx_set(0);
	k_busy_wait(SETTLE_MS * USEC_PER_MSEC / 2);
	rx_set(1);

	zassert_not_ok(k_sem_take(&rec.sem, NO_EVT_TIMEOUT), "dropout glitch reported");
	zassert_equal(1, rec_count());
}

ZTEST(zbook_ir, test_odd_edge_burst_reports_single_event)
{
	skip_without_settle();

	zassert_ok(zbook_ir_reg_cb(ZBOOK_IR_EVT_BOTH, rec_cb, NULL));

	for (int i = 0; i < 9; i++) {
		rx_set((i & 1) == 0);
		k_busy_wait(300);
	}

	zassert_ok(k_sem_take(&rec.sem, WAIT_TIMEOUT));
	zassert_not_ok(k_sem_take(&rec.sem, NO_EVT_TIMEOUT), "more than one event for a burst");
	zassert_equal(1, rec_count());
	zassert_equal(ZBOOK_IR_EVT_DETECTED, rec.log[0]);
}

ZTEST(zbook_ir, test_edge_during_slow_callback_is_not_lost)
{
	zassert_ok(zbook_ir_reg_cb(ZBOOK_IR_EVT_BOTH, rec_cb, NULL));
	rec.sleep_ms = SLOW_CB_MS;

	rx_set(1);
	zassert_ok(k_sem_take(&rec.entered, WAIT_TIMEOUT));

	rx_set(0);
	zassert_ok(k_sem_take(&rec.sem, WAIT_TIMEOUT));
	rec.sleep_ms = 0;

	zassert_ok(k_sem_take(&rec.sem, WAIT_TIMEOUT), "LOST during a slow callback was lost");
	zassert_equal(ZBOOK_IR_EVT_LOST, rec.log[1]);
	zassert_equal(2, rec_count());
}

ZTEST(zbook_ir, test_rm_cb_stops_further_callbacks)
{
	zassert_ok(zbook_ir_reg_cb(ZBOOK_IR_EVT_BOTH, rec_cb, NULL));

	rx_set(1);
	zassert_ok(k_sem_take(&rec.sem, WAIT_TIMEOUT));

	zassert_ok(zbook_ir_rm_cb());

	rx_set(0);
	zassert_not_ok(k_sem_take(&rec.sem, NO_EVT_TIMEOUT), "callback fired after rm_cb()");
	zassert_equal(1, rec_count());
}

ZTEST(zbook_ir, test_rm_cb_waits_for_running_callback)
{
	zassert_ok(zbook_ir_reg_cb(ZBOOK_IR_EVT_BOTH, rec_cb, NULL));
	rec.sleep_ms = SLOW_CB_MS;

	rx_set(1);
	zassert_ok(k_sem_take(&rec.entered, WAIT_TIMEOUT));
	zassert_false(rec.done, "test setup: callback not running");

	zassert_ok(zbook_ir_rm_cb());
	zassert_true(rec.done, "rm_cb() returned while the callback was still running");
}

ZTEST(zbook_ir, test_rm_cb_while_event_pending_keeps_tracking_level)
{
	skip_without_settle();

	zassert_ok(zbook_ir_reg_cb(ZBOOK_IR_EVT_BOTH, rec_cb, NULL));

	rx_set(1);
	zassert_ok(zbook_ir_rm_cb());
	zassert_not_ok(k_sem_take(&rec.sem, NO_EVT_TIMEOUT), "callback fired after rm_cb()");

	zassert_ok(zbook_ir_reg_cb(ZBOOK_IR_EVT_BOTH, rec_cb, NULL));
	rx_set(0);
	zassert_ok(k_sem_take(&rec.sem, WAIT_TIMEOUT));
	zassert_equal(ZBOOK_IR_EVT_LOST, rec.log[0], "level not tracked while unregistered");
}

ZTEST(zbook_ir, test_reg_cb_while_event_pending_uses_new_callback)
{
	skip_without_settle();

	zassert_ok(zbook_ir_reg_cb(ZBOOK_IR_EVT_BOTH, other_cb, NULL));

	rx_set(1);
	zassert_ok(zbook_ir_reg_cb(ZBOOK_IR_EVT_BOTH, rec_cb, NULL));

	zassert_ok(k_sem_take(&rec.sem, WAIT_TIMEOUT));
	zassert_equal(0, other_count);
}

ZTEST(zbook_ir, test_reg_cb_replaces_previous_callback)
{
	zassert_ok(zbook_ir_reg_cb(ZBOOK_IR_EVT_BOTH, other_cb, NULL));
	zassert_ok(zbook_ir_reg_cb(ZBOOK_IR_EVT_LOST, rec_cb, NULL));

	rx_set(1);
	zassert_not_ok(k_sem_take(&rec.sem, NO_EVT_TIMEOUT));

	rx_set(0);
	zassert_ok(k_sem_take(&rec.sem, WAIT_TIMEOUT));

	zassert_equal(1, rec_count());
	zassert_equal(0, other_count, "replaced callback still fired");
}

static const char user_data_expected[] = "sample";
static const void *user_data_received;

static void user_data_cb(enum zbook_ir_evt evt, void *user_data)
{
	user_data_received = user_data;
	rec_cb(evt, NULL);
}

ZTEST(zbook_ir, test_user_data_is_forwarded)
{
	user_data_received = NULL;

	zassert_ok(
		zbook_ir_reg_cb(ZBOOK_IR_EVT_DETECTED, user_data_cb, (void *)user_data_expected));

	rx_set(1);
	zassert_ok(k_sem_take(&rec.sem, WAIT_TIMEOUT));
	zassert_equal_ptr(user_data_expected, user_data_received);
}

ZTEST(zbook_ir, test_rm_cb_from_inside_callback)
{
	zassert_ok(zbook_ir_reg_cb(ZBOOK_IR_EVT_BOTH, rec_cb, NULL));
	rec.action = ACT_RM;

	rx_set(1);
	zassert_ok(k_sem_take(&rec.sem, WAIT_TIMEOUT));
	zassert_ok(rec.action_ret);

	rx_set(0);
	zassert_not_ok(k_sem_take(&rec.sem, NO_EVT_TIMEOUT), "callback fired after rm inside cb");
}

ZTEST(zbook_ir, test_reg_cb_from_inside_callback)
{
	zassert_ok(zbook_ir_reg_cb(ZBOOK_IR_EVT_BOTH, rec_cb, NULL));
	rec.action = ACT_REG_LOST;

	rx_set(1);
	zassert_ok(k_sem_take(&rec.sem, WAIT_TIMEOUT));
	zassert_ok(rec.action_ret);

	rx_set(0);
	zassert_ok(k_sem_take(&rec.sem, WAIT_TIMEOUT));
	zassert_equal(ZBOOK_IR_EVT_LOST, rec.log[1]);
}

ZTEST(zbook_ir, test_init_from_inside_callback)
{
	zassert_ok(zbook_ir_reg_cb(ZBOOK_IR_EVT_BOTH, rec_cb, NULL));
	rec.action = ACT_INIT;

	rx_set(1);
	zassert_ok(k_sem_take(&rec.sem, WAIT_TIMEOUT));
	zassert_ok(rec.action_ret);

	rx_set(0);
	zassert_ok(k_sem_take(&rec.sem, WAIT_TIMEOUT), "events dead after init inside cb");
	zassert_equal(ZBOOK_IR_EVT_LOST, rec.log[1]);
}

ZTEST(zbook_ir, test_pulse_from_inside_callback)
{
	zassert_ok(zbook_ir_reg_cb(ZBOOK_IR_EVT_BOTH, rec_cb, NULL));
	rec.action = ACT_PULSE;

	rx_set(1);
	zassert_ok(k_sem_take(&rec.sem, WAIT_TIMEOUT));
	zassert_ok(rec.action_ret);
	zassert_equal(1, emitter_logical());

	k_msleep(PULSE_MS + PULSE_MARGIN_MS);
	zassert_equal(0, emitter_logical());
}

ZTEST(zbook_ir, test_reinit_turns_emitter_off_and_cancels_pulse)
{
	zassert_ok(zbook_ir_emitter_on());
	zassert_ok(zbook_ir_init());
	zassert_equal(0, emitter_logical());
	zassert_equal(0, zbook_ir_emitter_get());

	zassert_ok(zbook_ir_emitter_pulse(PULSE_MS));
	zassert_ok(zbook_ir_init());
	zassert_equal(0, emitter_logical());

	zassert_ok(zbook_ir_emitter_on());
	k_msleep(PULSE_MS + PULSE_MARGIN_MS);
	zassert_equal(1, emitter_logical());
}

ZTEST(zbook_ir, test_reinit_keeps_callback_without_duplicates)
{
	zassert_ok(zbook_ir_reg_cb(ZBOOK_IR_EVT_BOTH, rec_cb, NULL));

	zassert_ok(zbook_ir_init());
	zassert_ok(zbook_ir_init());

	rx_set(1);
	zassert_ok(k_sem_take(&rec.sem, WAIT_TIMEOUT), "callback lost on re-init");
	zassert_not_ok(k_sem_take(&rec.sem, NO_EVT_TIMEOUT), "duplicated event after re-init");
	zassert_equal(1, rec_count());
}

ZTEST(zbook_ir, test_reinit_with_pending_edge_keeps_alternation)
{
	zassert_ok(zbook_ir_reg_cb(ZBOOK_IR_EVT_BOTH, rec_cb, NULL));

	rx_set(1);
	zassert_ok(k_sem_take(&rec.sem, WAIT_TIMEOUT));
	zassert_equal(ZBOOK_IR_EVT_DETECTED, rec.log[0]);

	rx_set(0);
	zassert_ok(zbook_ir_init());
	zassert_ok(k_sem_take(&rec.sem, WAIT_TIMEOUT), "LOST across re-init not reported");
	zassert_equal(ZBOOK_IR_EVT_LOST, rec.log[1]);

	rx_set(1);
	zassert_ok(k_sem_take(&rec.sem, WAIT_TIMEOUT));
	zassert_equal(ZBOOK_IR_EVT_DETECTED, rec.log[2]);

	zassert_not_ok(k_sem_take(&rec.sem, NO_EVT_TIMEOUT));
	zassert_equal(3, rec_count());
}

ZTEST(zbook_ir, test_reinit_while_ir_present_reports_lost)
{
	zassert_ok(zbook_ir_reg_cb(ZBOOK_IR_EVT_BOTH, rec_cb, NULL));

	rx_set(1);
	zassert_ok(k_sem_take(&rec.sem, WAIT_TIMEOUT));

	zassert_ok(zbook_ir_init());
	zassert_not_ok(k_sem_take(&rec.sem, NO_EVT_TIMEOUT), "re-init reported an event");

	rx_set(0);
	zassert_ok(k_sem_take(&rec.sem, WAIT_TIMEOUT), "LOST after re-init missing");
	zassert_equal(ZBOOK_IR_EVT_LOST, rec.log[1]);
	zassert_equal(2, rec_count());
}
