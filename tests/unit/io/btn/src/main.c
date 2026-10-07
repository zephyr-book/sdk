/************************************************************************
 * @file main.c
 *
 * @brief Unit tests for the Zbook button interface.
 * @author João Matheus Nascimento Dias (joao.dias@edge.ufal.br)
 *
 * @copyright Centro de Inovação EDGE 2026. Todos os direitos reservados.
 *
 ***********************************************************************/

#include <zephyr/ztest.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>

#include "io/zbook_btn.h"

#define WAIT_TIMEOUT      K_MSEC(100)
#define LONG_PRESS_TIMEOUT K_MSEC(200)

#define LVL_RELEASED 1
#define LVL_PRESSED  0

static const struct gpio_dt_spec btn0 = GPIO_DT_SPEC_GET(DT_ALIAS(button0), gpios);
static const struct gpio_dt_spec btn1 = GPIO_DT_SPEC_GET(DT_ALIAS(button1), gpios);
static const struct gpio_dt_spec btn2 = GPIO_DT_SPEC_GET(DT_ALIAS(button2), gpios);
static const struct gpio_dt_spec btn3 = GPIO_DT_SPEC_GET(DT_ALIAS(button3), gpios);

static struct {
	struct k_sem sem;
	enum zbook_btn last_btn;
	enum zbook_btn_evt last_evt;
	int count;
} rec;

static void rec_reset(void)
{
	k_sem_init(&rec.sem, 0, 10);
	rec.last_btn = ZBOOK_BTN_ALL;
	rec.last_evt = ZBOOK_BTN_EVT_NONE;
	rec.count = 0;
}

static void rec_cb(enum zbook_btn btn, enum zbook_btn_evt evt, void *user_data)
{
	ARG_UNUSED(user_data);

	rec.last_btn = btn;
	rec.last_evt = evt;
	rec.count++;
	k_sem_give(&rec.sem);
}

static void set_all_released(void)
{
	gpio_emul_input_set_dt(&btn0, LVL_RELEASED);
	gpio_emul_input_set_dt(&btn1, LVL_RELEASED);
	gpio_emul_input_set_dt(&btn2, LVL_RELEASED);
	gpio_emul_input_set_dt(&btn3, LVL_RELEASED);
}

static void *suite_setup(void)
{
	zassert_equal(zbook_btn_init(), 0, "zbook_btn_init() failed");

	return NULL;
}

static void test_before(void *fixture)
{
	ARG_UNUSED(fixture);

	set_all_released();

	k_msleep(30);

	zassert_equal(zbook_btn_rm_cb(ZBOOK_BTN_ALL), 0);
	rec_reset();
}

ZTEST_SUITE(zbook_btn, NULL, suite_setup, test_before, NULL, NULL);

ZTEST(zbook_btn, test_reg_cb_rejects_invalid_button)
{
	/* Exercises both sides of IS_BTN_VALID's range check: too high and
	 * negative.
	 */
	zassert_equal(zbook_btn_reg_cb((enum zbook_btn)123, ZBOOK_BTN_EVT_BOTH, rec_cb, NULL),
		      -EINVAL);
	zassert_equal(zbook_btn_reg_cb((enum zbook_btn) - 1, ZBOOK_BTN_EVT_BOTH, rec_cb, NULL),
		      -EINVAL);
}

ZTEST(zbook_btn, test_reg_cb_rejects_invalid_event)
{
	/* Exercises IS_BTN_EVT_VALID: bits outside ZBOOK_BTN_EVT_ALL, and the
	 * empty mask.
	 */
	zassert_equal(zbook_btn_reg_cb(ZBOOK_BTN_0, (enum zbook_btn_evt)0x5A5A, rec_cb, NULL),
		      -EINVAL);
	zassert_equal(zbook_btn_reg_cb(ZBOOK_BTN_0, ZBOOK_BTN_EVT_NONE, rec_cb, NULL), -EINVAL);
}

ZTEST(zbook_btn, test_reg_cb_rejects_null_callback)
{
	zassert_equal(zbook_btn_reg_cb(ZBOOK_BTN_0, ZBOOK_BTN_EVT_BOTH, NULL, NULL), -EINVAL);
}

ZTEST(zbook_btn, test_rm_cb_rejects_invalid_button)
{
	/* Exercises both sides of IS_BTN_VALID's range check. */
	zassert_equal(zbook_btn_rm_cb((enum zbook_btn)123), -EINVAL);
	zassert_equal(zbook_btn_rm_cb((enum zbook_btn) - 1), -EINVAL);
}

ZTEST(zbook_btn, test_rm_cb_on_unregistered_button_succeeds)
{
	/* No hardware state to fail on anymore: removing a callback that was
	 * never registered is a no-op.
	 */
	zassert_equal(zbook_btn_rm_cb(ZBOOK_BTN_1), 0);
}

ZTEST(zbook_btn, test_pressed_fires_only_on_press)
{
	zassert_equal(zbook_btn_reg_cb(ZBOOK_BTN_0, ZBOOK_BTN_EVT_PRESSED, rec_cb, NULL), 0);

	gpio_emul_input_set_dt(&btn0, LVL_PRESSED);
	zassert_equal(k_sem_take(&rec.sem, WAIT_TIMEOUT), 0, "callback not fired on press");
	zassert_equal(rec.last_btn, ZBOOK_BTN_0);
	zassert_equal(rec.last_evt, ZBOOK_BTN_EVT_PRESSED);
	zassert_equal(rec.count, 1);

	gpio_emul_input_set_dt(&btn0, LVL_RELEASED);
	zassert_not_equal(k_sem_take(&rec.sem, K_MSEC(20)), 0, "callback fired on release");
	zassert_equal(rec.count, 1);

	zassert_equal(zbook_btn_rm_cb(ZBOOK_BTN_0), 0);
}

ZTEST(zbook_btn, test_released_fires_only_on_release)
{
	zassert_equal(zbook_btn_reg_cb(ZBOOK_BTN_1, ZBOOK_BTN_EVT_RELEASED, rec_cb, NULL), 0);

	gpio_emul_input_set_dt(&btn1, LVL_PRESSED);
	zassert_not_equal(k_sem_take(&rec.sem, K_MSEC(20)), 0, "callback fired on press");
	zassert_equal(rec.count, 0);

	gpio_emul_input_set_dt(&btn1, LVL_RELEASED);
	zassert_equal(k_sem_take(&rec.sem, WAIT_TIMEOUT), 0, "callback not fired on release");
	zassert_equal(rec.last_btn, ZBOOK_BTN_1);
	zassert_equal(rec.last_evt, ZBOOK_BTN_EVT_RELEASED);
	zassert_equal(rec.count, 1);

	zassert_equal(zbook_btn_rm_cb(ZBOOK_BTN_1), 0);
}

ZTEST(zbook_btn, test_both_fires_on_press_and_release)
{
	zassert_equal(zbook_btn_reg_cb(ZBOOK_BTN_2, ZBOOK_BTN_EVT_BOTH, rec_cb, NULL), 0);

	gpio_emul_input_set_dt(&btn2, LVL_PRESSED);
	zassert_equal(k_sem_take(&rec.sem, WAIT_TIMEOUT), 0, "callback not fired on press");
	zassert_equal(rec.last_evt, ZBOOK_BTN_EVT_PRESSED);

	gpio_emul_input_set_dt(&btn2, LVL_RELEASED);
	zassert_equal(k_sem_take(&rec.sem, WAIT_TIMEOUT), 0, "callback not fired on release");
	zassert_equal(rec.last_evt, ZBOOK_BTN_EVT_RELEASED);

	zassert_equal(rec.count, 2);

	zassert_equal(zbook_btn_rm_cb(ZBOOK_BTN_2), 0);
}

ZTEST(zbook_btn, test_all_registers_every_button)
{
	zassert_equal(zbook_btn_reg_cb(ZBOOK_BTN_ALL, ZBOOK_BTN_EVT_PRESSED, rec_cb, NULL), 0);

	gpio_emul_input_set_dt(&btn0, LVL_PRESSED);
	zassert_equal(k_sem_take(&rec.sem, WAIT_TIMEOUT), 0);
	zassert_equal(rec.last_btn, ZBOOK_BTN_0);

	gpio_emul_input_set_dt(&btn1, LVL_PRESSED);
	zassert_equal(k_sem_take(&rec.sem, WAIT_TIMEOUT), 0);
	zassert_equal(rec.last_btn, ZBOOK_BTN_1);

	gpio_emul_input_set_dt(&btn2, LVL_PRESSED);
	zassert_equal(k_sem_take(&rec.sem, WAIT_TIMEOUT), 0);
	zassert_equal(rec.last_btn, ZBOOK_BTN_2);

	gpio_emul_input_set_dt(&btn3, LVL_PRESSED);
	zassert_equal(k_sem_take(&rec.sem, WAIT_TIMEOUT), 0);
	zassert_equal(rec.last_btn, ZBOOK_BTN_3);

	zassert_equal(rec.count, 4);

	zassert_equal(zbook_btn_rm_cb(ZBOOK_BTN_ALL), 0);
}

ZTEST(zbook_btn, test_rm_cb_stops_further_callbacks)
{
	zassert_equal(zbook_btn_reg_cb(ZBOOK_BTN_2, ZBOOK_BTN_EVT_BOTH, rec_cb, NULL), 0);

	gpio_emul_input_set_dt(&btn2, LVL_PRESSED);
	zassert_equal(k_sem_take(&rec.sem, WAIT_TIMEOUT), 0);

	zassert_equal(zbook_btn_rm_cb(ZBOOK_BTN_2), 0);

	gpio_emul_input_set_dt(&btn2, LVL_RELEASED);
	zassert_not_equal(k_sem_take(&rec.sem, K_MSEC(20)), 0,
			  "callback fired after zbook_btn_rm_cb()");
	zassert_equal(rec.count, 1);
}

ZTEST(zbook_btn, test_long_pressed_fires_after_threshold)
{
	zassert_equal(zbook_btn_reg_cb(ZBOOK_BTN_0, ZBOOK_BTN_EVT_ALL, rec_cb, NULL), 0);

	gpio_emul_input_set_dt(&btn0, LVL_PRESSED);
	zassert_equal(k_sem_take(&rec.sem, WAIT_TIMEOUT), 0, "PRESSED not fired");
	zassert_equal(rec.last_evt, ZBOOK_BTN_EVT_PRESSED);

	zassert_equal(k_sem_take(&rec.sem, LONG_PRESS_TIMEOUT), 0, "LONG_PRESSED not fired");
	zassert_equal(rec.last_evt, ZBOOK_BTN_EVT_LONG_PRESSED);

	gpio_emul_input_set_dt(&btn0, LVL_RELEASED);
	zassert_equal(k_sem_take(&rec.sem, WAIT_TIMEOUT), 0, "RELEASED not fired");
	zassert_equal(rec.last_evt, ZBOOK_BTN_EVT_RELEASED);

	zassert_equal(rec.count, 3);

	zassert_equal(zbook_btn_rm_cb(ZBOOK_BTN_0), 0);
}

ZTEST(zbook_btn, test_long_pressed_not_fired_on_short_press)
{
	zassert_equal(zbook_btn_reg_cb(ZBOOK_BTN_1, ZBOOK_BTN_EVT_ALL, rec_cb, NULL), 0);

	gpio_emul_input_set_dt(&btn1, LVL_PRESSED);
	zassert_equal(k_sem_take(&rec.sem, WAIT_TIMEOUT), 0, "PRESSED not fired");

	gpio_emul_input_set_dt(&btn1, LVL_RELEASED);
	zassert_equal(k_sem_take(&rec.sem, WAIT_TIMEOUT), 0, "RELEASED not fired");

	/* Released well before CONFIG_BTN_LONG_PRESS_MS: the pending timer
	 * must have been cancelled.
	 */
	zassert_not_equal(k_sem_take(&rec.sem, LONG_PRESS_TIMEOUT), 0,
			  "LONG_PRESSED fired after early release");
	zassert_equal(rec.count, 2);

	zassert_equal(zbook_btn_rm_cb(ZBOOK_BTN_1), 0);
}

ZTEST(zbook_btn, test_rm_cb_cancels_pending_long_press)
{
	zassert_equal(zbook_btn_reg_cb(ZBOOK_BTN_2, ZBOOK_BTN_EVT_LONG_PRESSED, rec_cb, NULL), 0);

	gpio_emul_input_set_dt(&btn2, LVL_PRESSED);

	zassert_equal(zbook_btn_rm_cb(ZBOOK_BTN_2), 0);

	zassert_not_equal(k_sem_take(&rec.sem, LONG_PRESS_TIMEOUT), 0,
			  "LONG_PRESSED fired after zbook_btn_rm_cb()");
	zassert_equal(rec.count, 0);
}

static const char user_data_expected[] = "sample";
static const void *user_data_received;

static void user_data_cb(enum zbook_btn btn, enum zbook_btn_evt evt, void *user_data)
{
	user_data_received = user_data;
	rec_cb(btn, evt, NULL);
}

ZTEST(zbook_btn, test_user_data_is_forwarded)
{
	user_data_received = NULL;

	zassert_equal(zbook_btn_reg_cb(ZBOOK_BTN_0, ZBOOK_BTN_EVT_PRESSED, user_data_cb,
				       (void *)user_data_expected),
		      0);

	gpio_emul_input_set_dt(&btn0, LVL_PRESSED);
	zassert_equal(k_sem_take(&rec.sem, WAIT_TIMEOUT), 0);
	zassert_equal_ptr(user_data_received, user_data_expected);

	zassert_equal(zbook_btn_rm_cb(ZBOOK_BTN_0), 0);
}
