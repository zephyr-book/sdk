#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/ztest.h>

#include "io/zbook_pin.h"

#define PORT DEVICE_DT_GET(DT_NODELABEL(gpio0))

#define IO01 0
#define IO39 1
#define IO45 2
#define IO46 3

#define WAIT_DEBOUNCE() k_sleep(K_MSEC(2 * CONFIG_ZBOOK_INTERFACE_IO_GPIO_DEBOUNCE_MS))

static const enum zbook_pin_header headers[] = {
	ZBOOK_PIN_HEADER_IO_01,
	ZBOOK_PIN_HEADER_IO_39,
	ZBOOK_PIN_HEADER_IO_45,
	ZBOOK_PIN_HEADER_IO_46,
};

static struct zbook_pin *handles[ARRAY_SIZE(headers)];

static atomic_t cb_count;
static enum zbook_pin_header cb_pin;
static void *cb_user_data;
static int cb_read_ret;
static uint8_t cb_read_level;

static int set(int ch, struct zbook_pin_header_config cfg)
{
	return zbook_pin_set_function(headers[ch], &cfg, &handles[ch]);
}

static int write_state(int ch, uint8_t state)
{
	return zbook_pin_write(handles[ch], &state, 1);
}

static void test_cb(enum zbook_pin_header pin, void *user_data)
{
	cb_pin = pin;
	cb_user_data = user_data;
	cb_read_ret = zbook_pin_read(handles[IO39], &cb_read_level, 1);
	atomic_inc(&cb_count);
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	atomic_set(&cb_count, 0);
	cb_pin = 0;
	cb_user_data = NULL;
	cb_read_ret = -1;
	cb_read_level = 0xFF;
}

static void after(void *fixture)
{
	ARG_UNUSED(fixture);

	for (size_t i = 0; i < ARRAY_SIZE(handles); i++) {
		if (handles[i] != NULL) {
			zbook_pin_release(handles[i]);
			handles[i] = NULL;
		}
	}
}

ZTEST_SUITE(zbook_io_gpio, NULL, NULL, before, after, NULL);

ZTEST(zbook_io_gpio, test_output_starts_at_requested_state)
{
	zassert_ok(set(IO01, ZBOOK_PIN_CFG_GPIO_OUT(ZBOOK_PIN_HEADER_STATE_HIGH)));
	zassert_equal(1, gpio_emul_output_get(PORT, IO01));

	zassert_ok(set(IO01, ZBOOK_PIN_CFG_GPIO_OUT(ZBOOK_PIN_HEADER_STATE_LOW)));
	zassert_equal(0, gpio_emul_output_get(PORT, IO01));
}

ZTEST(zbook_io_gpio, test_write_drives_output)
{
	zassert_ok(set(IO01, ZBOOK_PIN_CFG_GPIO_OUT(ZBOOK_PIN_HEADER_STATE_LOW)));

	zassert_ok(write_state(IO01, ZBOOK_PIN_HEADER_STATE_HIGH));
	zassert_equal(1, gpio_emul_output_get(PORT, IO01));

	zassert_ok(write_state(IO01, ZBOOK_PIN_HEADER_STATE_LOW));
	zassert_equal(0, gpio_emul_output_get(PORT, IO01));
}

ZTEST(zbook_io_gpio, test_output_reads_back_its_level)
{
	uint8_t level;
	gpio_flags_t flags;

	zassert_ok(set(IO01, ZBOOK_PIN_CFG_GPIO_OUT(ZBOOK_PIN_HEADER_STATE_LOW)));
	zassert_ok(gpio_emul_flags_get(PORT, IO01, &flags));
	zassert_true(flags & GPIO_INPUT);

	zassert_ok(write_state(IO01, ZBOOK_PIN_HEADER_STATE_HIGH));
	zassert_ok(zbook_pin_read(handles[IO01], &level, 1));
	zassert_equal(ZBOOK_PIN_HEADER_STATE_HIGH, level);

	zassert_ok(write_state(IO01, ZBOOK_PIN_HEADER_STATE_LOW));
	zassert_ok(zbook_pin_read(handles[IO01], &level, 1));
	zassert_equal(ZBOOK_PIN_HEADER_STATE_LOW, level);
}

ZTEST(zbook_io_gpio, test_write_rejects_bad_len_and_state)
{
	uint8_t buf[2] = {ZBOOK_PIN_HEADER_STATE_HIGH, 0};

	zassert_ok(set(IO01, ZBOOK_PIN_CFG_GPIO_OUT(ZBOOK_PIN_HEADER_STATE_LOW)));

	zassert_equal(-EINVAL, zbook_pin_write(handles[IO01], buf, 2));
	zassert_equal(-EINVAL, write_state(IO01, ZBOOK_PIN_HEADER_STATE_OPEN_DRAIN));
	zassert_equal(-EINVAL, write_state(IO01, 7));
	zassert_equal(0, gpio_emul_output_get(PORT, IO01));
}

ZTEST(zbook_io_gpio, test_write_on_input_is_not_supported)
{
	zassert_ok(set(IO01, ZBOOK_PIN_CFG_GPIO_IN(ZBOOK_PIN_HEADER_PULL_NONE)));
	zassert_equal(-ENOTSUP, write_state(IO01, ZBOOK_PIN_HEADER_STATE_HIGH));
}

ZTEST(zbook_io_gpio, test_read_follows_input)
{
	uint8_t level;
	uint8_t buf[2];

	zassert_ok(set(IO01, ZBOOK_PIN_CFG_GPIO_IN(ZBOOK_PIN_HEADER_PULL_NONE)));

	zassert_ok(gpio_emul_input_set(PORT, IO01, 1));
	zassert_ok(zbook_pin_read(handles[IO01], &level, 1));
	zassert_equal(ZBOOK_PIN_HEADER_STATE_HIGH, level);

	zassert_ok(gpio_emul_input_set(PORT, IO01, 0));
	zassert_ok(zbook_pin_read(handles[IO01], &level, 1));
	zassert_equal(ZBOOK_PIN_HEADER_STATE_LOW, level);

	zassert_equal(-EINVAL, zbook_pin_read(handles[IO01], buf, 2));
}

ZTEST(zbook_io_gpio, test_pull_is_applied)
{
	gpio_flags_t flags;

	zassert_ok(set(IO01, ZBOOK_PIN_CFG_GPIO_IN(ZBOOK_PIN_HEADER_PULL_UP)));
	zassert_ok(gpio_emul_flags_get(PORT, IO01, &flags));
	zassert_true(flags & GPIO_PULL_UP);

	zassert_ok(set(IO01, ZBOOK_PIN_CFG_GPIO_IN(ZBOOK_PIN_HEADER_PULL_DOWN)));
	zassert_ok(gpio_emul_flags_get(PORT, IO01, &flags));
	zassert_true(flags & GPIO_PULL_DOWN);
	zassert_false(flags & GPIO_PULL_UP);
}

ZTEST(zbook_io_gpio, test_header_pin_maps_to_its_channel)
{
	zassert_ok(set(IO45, ZBOOK_PIN_CFG_GPIO_OUT(ZBOOK_PIN_HEADER_STATE_HIGH)));
	zassert_equal(1, gpio_emul_output_get(PORT, IO45));
	zassert_equal(0, gpio_emul_output_get(PORT, IO01));
}

ZTEST(zbook_io_gpio, test_disabled_channel_only_takes_its_own_pin_out)
{
	enum zbook_pin_header_function function;

	zassert_equal(-ENODEV, set(IO46, ZBOOK_PIN_CFG_GPIO_OUT(ZBOOK_PIN_HEADER_STATE_HIGH)));
	zassert_equal(-ENODATA, zbook_pin_get_function(ZBOOK_PIN_HEADER_IO_46, &function));

	zassert_ok(set(IO45, ZBOOK_PIN_CFG_GPIO_OUT(ZBOOK_PIN_HEADER_STATE_HIGH)));
	zassert_equal(1, gpio_emul_output_get(PORT, IO45));
}

ZTEST(zbook_io_gpio, test_invalid_cfg_leaves_pin_unconfigured)
{
	enum zbook_pin_header_function function;
	struct zbook_pin_header_config bad_pull = ZBOOK_PIN_CFG_GPIO_IN(ZBOOK_PIN_HEADER_PULL_NONE);
	struct zbook_pin_header_config irq_on_output =
		ZBOOK_PIN_CFG_GPIO_OUT(ZBOOK_PIN_HEADER_STATE_LOW);
	struct zbook_pin_header_config irq_no_cb = ZBOOK_PIN_CFG_GPIO_IN_IRQ(
		ZBOOK_PIN_HEADER_PULL_NONE, ZBOOK_PIN_HEADER_INTERRUPT_RISING, NULL, NULL);
	struct zbook_pin_header_config bad_edge = ZBOOK_PIN_CFG_GPIO_IN_IRQ(
		ZBOOK_PIN_HEADER_PULL_NONE, ZBOOK_PIN_HEADER_INTERRUPT_RISING, test_cb, NULL);

	bad_pull.cfg.gpio.pull = 99;
	irq_on_output.cfg.gpio.interrupt = ZBOOK_PIN_HEADER_INTERRUPT_RISING;
	irq_on_output.cfg.gpio.callback = test_cb;
	bad_edge.cfg.gpio.interrupt = 9;

	zassert_equal(-EINVAL, set(IO01, bad_pull));
	zassert_equal(-EINVAL, set(IO01, irq_on_output));
	zassert_equal(-EINVAL, set(IO01, irq_no_cb));
	zassert_equal(-EINVAL, set(IO01, bad_edge));
	zassert_equal(-ENODATA, zbook_pin_get_function(ZBOOK_PIN_HEADER_IO_01, &function));
}

ZTEST(zbook_io_gpio, test_driver_error_leaves_pin_unconfigured)
{
	enum zbook_pin_header_function function;

	/* gpio_emul has no open-drain support, the rp2350 driver does. */
	zassert_equal(-ENOTSUP,
		      set(IO01, ZBOOK_PIN_CFG_GPIO_OUT(ZBOOK_PIN_HEADER_STATE_OPEN_DRAIN)));
	zassert_equal(-ENODATA, zbook_pin_get_function(ZBOOK_PIN_HEADER_IO_01, &function));
}

ZTEST(zbook_io_gpio, test_release_returns_pin_to_input_without_pull)
{
	gpio_flags_t flags;
	struct zbook_pin_header_config cfg = ZBOOK_PIN_CFG_GPIO_OUT(ZBOOK_PIN_HEADER_STATE_HIGH);

	cfg.cfg.gpio.pull = ZBOOK_PIN_HEADER_PULL_UP;
	zassert_ok(set(IO01, cfg));
	zassert_ok(zbook_pin_release(handles[IO01]));

	zassert_ok(gpio_emul_flags_get(PORT, IO01, &flags));
	zassert_true(flags & GPIO_INPUT);
	zassert_false(flags & GPIO_OUTPUT);
	zassert_false(flags & (GPIO_PULL_UP | GPIO_PULL_DOWN));
}

ZTEST(zbook_io_gpio, test_irq_fires_once_after_debounce)
{
	zassert_ok(set(IO39, ZBOOK_PIN_CFG_GPIO_IN_IRQ(ZBOOK_PIN_HEADER_PULL_NONE,
						       ZBOOK_PIN_HEADER_INTERRUPT_RISING, test_cb,
						       (void *)0x1234)));
	zassert_ok(gpio_emul_input_set(PORT, IO39, 0));

	/* Bounces inside the debounce window are masked out. */
	zassert_ok(gpio_emul_input_set(PORT, IO39, 1));
	zassert_ok(gpio_emul_input_set(PORT, IO39, 0));
	zassert_ok(gpio_emul_input_set(PORT, IO39, 1));
	WAIT_DEBOUNCE();

	zassert_equal(1, atomic_get(&cb_count));
	zassert_equal(ZBOOK_PIN_HEADER_IO_39, cb_pin);
	zassert_equal_ptr((void *)0x1234, cb_user_data);
}

ZTEST(zbook_io_gpio, test_irq_rearms_after_debounce)
{
	zassert_ok(set(IO39, ZBOOK_PIN_CFG_GPIO_IN_IRQ(ZBOOK_PIN_HEADER_PULL_NONE,
						       ZBOOK_PIN_HEADER_INTERRUPT_RISING, test_cb,
						       NULL)));
	zassert_ok(gpio_emul_input_set(PORT, IO39, 0));

	zassert_ok(gpio_emul_input_set(PORT, IO39, 1));
	WAIT_DEBOUNCE();
	zassert_ok(gpio_emul_input_set(PORT, IO39, 0));
	zassert_ok(gpio_emul_input_set(PORT, IO39, 1));
	WAIT_DEBOUNCE();

	zassert_equal(2, atomic_get(&cb_count));
}

ZTEST(zbook_io_gpio, test_irq_only_on_requested_edge)
{
	zassert_ok(set(IO39, ZBOOK_PIN_CFG_GPIO_IN_IRQ(ZBOOK_PIN_HEADER_PULL_NONE,
						       ZBOOK_PIN_HEADER_INTERRUPT_FALLING, test_cb,
						       NULL)));
	zassert_ok(gpio_emul_input_set(PORT, IO39, 1));

	zassert_ok(gpio_emul_input_set(PORT, IO39, 0));
	WAIT_DEBOUNCE();
	zassert_equal(1, atomic_get(&cb_count));

	zassert_ok(gpio_emul_input_set(PORT, IO39, 1));
	WAIT_DEBOUNCE();
	zassert_equal(1, atomic_get(&cb_count));
}

ZTEST(zbook_io_gpio, test_callback_can_read_its_pin)
{
	zassert_ok(set(IO39,
		       ZBOOK_PIN_CFG_GPIO_IN_IRQ(ZBOOK_PIN_HEADER_PULL_NONE,
						 ZBOOK_PIN_HEADER_INTERRUPT_BOTH, test_cb, NULL)));
	zassert_ok(gpio_emul_input_set(PORT, IO39, 0));

	zassert_ok(gpio_emul_input_set(PORT, IO39, 1));
	WAIT_DEBOUNCE();

	zassert_equal(1, atomic_get(&cb_count));
	zassert_ok(cb_read_ret);
	zassert_equal(ZBOOK_PIN_HEADER_STATE_HIGH, cb_read_level);
}

ZTEST(zbook_io_gpio, test_release_stops_irq)
{
	zassert_ok(set(IO39, ZBOOK_PIN_CFG_GPIO_IN_IRQ(ZBOOK_PIN_HEADER_PULL_NONE,
						       ZBOOK_PIN_HEADER_INTERRUPT_RISING, test_cb,
						       NULL)));
	zassert_ok(gpio_emul_input_set(PORT, IO39, 0));
	zassert_ok(zbook_pin_release(handles[IO39]));

	zassert_ok(gpio_emul_input_set(PORT, IO39, 1));
	WAIT_DEBOUNCE();

	zassert_equal(0, atomic_get(&cb_count));
}

ZTEST(zbook_io_gpio, test_release_cancels_pending_callback)
{
	zassert_ok(set(IO39, ZBOOK_PIN_CFG_GPIO_IN_IRQ(ZBOOK_PIN_HEADER_PULL_NONE,
						       ZBOOK_PIN_HEADER_INTERRUPT_RISING, test_cb,
						       NULL)));
	zassert_ok(gpio_emul_input_set(PORT, IO39, 0));

	zassert_ok(gpio_emul_input_set(PORT, IO39, 1));
	zassert_ok(zbook_pin_release(handles[IO39]));
	WAIT_DEBOUNCE();

	zassert_equal(0, atomic_get(&cb_count));
}

ZTEST(zbook_io_gpio, test_reconfigure_without_irq_stops_irq)
{
	zassert_ok(set(IO39, ZBOOK_PIN_CFG_GPIO_IN_IRQ(ZBOOK_PIN_HEADER_PULL_NONE,
						       ZBOOK_PIN_HEADER_INTERRUPT_RISING, test_cb,
						       NULL)));
	zassert_ok(gpio_emul_input_set(PORT, IO39, 0));
	zassert_ok(set(IO39, ZBOOK_PIN_CFG_GPIO_IN(ZBOOK_PIN_HEADER_PULL_NONE)));

	zassert_ok(gpio_emul_input_set(PORT, IO39, 1));
	WAIT_DEBOUNCE();

	zassert_equal(0, atomic_get(&cb_count));
}
