#include <zephyr/ztest.h>
#include <zephyr/fff.h>
#include <zephyr/drivers/pwm/pwm_fake.h>

#include "actuators/zbook_buzzer.h"

DEFINE_FFF_GLOBALS;

#define DEFAULT_FREQUENCY_HZ 2000U

static struct {
	int on;
	int off;
	int set_volume;
	int set_frequency;
	int beep_ms;
	int beep_repeated;
	int beep;
	int beep_full;
	int init_pwm_fail;
	int on_after_failed_init;
} pre_init;

static void *zbook_buzzer_setup(void)
{
	static bool captured;

	if (captured) {
		return NULL;
	}
	captured = true;

	pre_init.on = zbook_buzzer_on();
	pre_init.off = zbook_buzzer_off();
	pre_init.set_volume = zbook_buzzer_set_volume(50);
	pre_init.set_frequency = zbook_buzzer_set_frequency(1000);
	pre_init.beep_ms = zbook_buzzer_beep_ms(1);
	pre_init.beep_repeated = zbook_buzzer_beep_repeated(1);
	pre_init.beep = zbook_buzzer_beep(ZBOOK_BUZZER_BEEP_DURATION_SHORT);
	pre_init.beep_full = zbook_buzzer_beep_full(1, 1);

	fake_pwm_set_cycles_fake.return_val = -EIO;
	pre_init.init_pwm_fail = zbook_buzzer_init();
	RESET_FAKE(fake_pwm_set_cycles);
	pre_init.on_after_failed_init = zbook_buzzer_on();

	return NULL;
}

static void zbook_buzzer_before(void *fixture)
{
	ARG_UNUSED(fixture);

	if (zbook_buzzer_init() == 0) {
		zbook_buzzer_set_volume(100);
		zbook_buzzer_set_frequency(DEFAULT_FREQUENCY_HZ);
	}

	RESET_FAKE(fake_pwm_set_cycles);
}

ZTEST_SUITE(zbook_buzzer, NULL, zbook_buzzer_setup, zbook_buzzer_before, NULL, NULL);

ZTEST(zbook_buzzer, test_00_not_ready_before_init)
{
	zassert_equal(-ENODEV, pre_init.on);
	zassert_equal(-ENODEV, pre_init.off);
	zassert_equal(-ENODEV, pre_init.set_volume);
	zassert_equal(-ENODEV, pre_init.set_frequency);
	zassert_equal(-ENODEV, pre_init.beep_ms);
	zassert_equal(-ENODEV, pre_init.beep_repeated);
	zassert_equal(-ENODEV, pre_init.beep);
	zassert_equal(-ENODEV, pre_init.beep_full);
}

ZTEST(zbook_buzzer, test_01_init_pwm_failure_leaves_not_ready)
{
	zassert_equal(-EIO, pre_init.init_pwm_fail);
	zassert_equal(-ENODEV, pre_init.on_after_failed_init);
}

ZTEST(zbook_buzzer, test_02_init_ok_starts_silent)
{
	zassert_ok(zbook_buzzer_init());

	zassert_equal(1, fake_pwm_set_cycles_fake.call_count);
	zassert_equal(0, fake_pwm_set_cycles_fake.arg3_val);
}

ZTEST(zbook_buzzer, test_03_on_sets_half_duty)
{
	/* 50% duty (not 100%, which is a constant DC level -- silent on a
	 * piezo buzzer): the loudest audible square wave, at volume 100.
	 */
	zassert_ok(zbook_buzzer_on());

	zassert_equal(1, fake_pwm_set_cycles_fake.call_count);
	zassert_equal(500000, fake_pwm_set_cycles_fake.arg2_val);
	zassert_equal(250000, fake_pwm_set_cycles_fake.arg3_val);
}

ZTEST(zbook_buzzer, test_04_set_volume_scales_duty)
{
	/* Volume 50 -> half of the audible 0-50% duty range -> 25% duty. */
	zassert_ok(zbook_buzzer_on());
	zassert_ok(zbook_buzzer_set_volume(50));

	zassert_equal(2, fake_pwm_set_cycles_fake.call_count);
	zassert_equal(125000, fake_pwm_set_cycles_fake.arg3_val);
}

ZTEST(zbook_buzzer, test_05_low_volume_is_not_silent)
{
	zassert_ok(zbook_buzzer_on());
	zassert_ok(zbook_buzzer_set_volume(1));

	zassert_equal(2500, fake_pwm_set_cycles_fake.arg3_val);
}

ZTEST(zbook_buzzer, test_06_off_sets_zero_duty)
{
	zassert_ok(zbook_buzzer_on());
	zassert_ok(zbook_buzzer_off());

	zassert_equal(2, fake_pwm_set_cycles_fake.call_count);
	zassert_equal(0, fake_pwm_set_cycles_fake.arg3_val);
}

ZTEST(zbook_buzzer, test_07_set_volume_invalid_args)
{
	zassert_equal(-EINVAL, zbook_buzzer_set_volume(101));
	zassert_equal(0, fake_pwm_set_cycles_fake.call_count);
}

ZTEST(zbook_buzzer, test_08_set_frequency_changes_period)
{
	zassert_ok(zbook_buzzer_on());
	zassert_ok(zbook_buzzer_set_frequency(1000));

	zassert_equal(1000000, fake_pwm_set_cycles_fake.arg2_val);
	zassert_equal(500000, fake_pwm_set_cycles_fake.arg3_val);
}

ZTEST(zbook_buzzer, test_09_set_frequency_invalid_args)
{
	zassert_equal(-EINVAL, zbook_buzzer_set_frequency(0));
	zassert_equal(-EINVAL, zbook_buzzer_set_frequency(NSEC_PER_SEC + 1U));
	zassert_equal(0, fake_pwm_set_cycles_fake.call_count);
}

ZTEST(zbook_buzzer, test_10_on_failure_stays_off)
{
	fake_pwm_set_cycles_fake.return_val = -EIO;
	zassert_equal(-EIO, zbook_buzzer_on());

	fake_pwm_set_cycles_fake.return_val = 0;
	zassert_ok(zbook_buzzer_set_volume(100));
	zassert_equal(0, fake_pwm_set_cycles_fake.arg3_val);
}

ZTEST(zbook_buzzer, test_11_set_volume_failure_rolls_back)
{
	zassert_ok(zbook_buzzer_on());

	fake_pwm_set_cycles_fake.return_val = -EIO;
	zassert_equal(-EIO, zbook_buzzer_set_volume(10));

	fake_pwm_set_cycles_fake.return_val = 0;
	zassert_ok(zbook_buzzer_on());
	zassert_equal(250000, fake_pwm_set_cycles_fake.arg3_val);
}

ZTEST(zbook_buzzer, test_12_set_frequency_failure_rolls_back)
{
	zassert_ok(zbook_buzzer_on());

	fake_pwm_set_cycles_fake.return_val = -EIO;
	zassert_equal(-EIO, zbook_buzzer_set_frequency(1000));

	fake_pwm_set_cycles_fake.return_val = 0;
	zassert_ok(zbook_buzzer_on());
	zassert_equal(500000, fake_pwm_set_cycles_fake.arg2_val);
}

ZTEST(zbook_buzzer, test_13_beep_ms_ends_off)
{
	zassert_ok(zbook_buzzer_beep_ms(5));

	zassert_equal(2, fake_pwm_set_cycles_fake.call_count);
	zassert_equal(0, fake_pwm_set_cycles_fake.arg3_val);
}

ZTEST(zbook_buzzer, test_14_beep_repeated_calls_multiple_times)
{
	/* Each beep is an on+off pair, so 3 repeats make 6 pwm_set() calls. */
	zassert_ok(zbook_buzzer_beep_repeated(3));

	zassert_equal(6, fake_pwm_set_cycles_fake.call_count);
	zassert_equal(0, fake_pwm_set_cycles_fake.arg3_val);
}

ZTEST(zbook_buzzer, test_15_beep_repeated_invalid_args)
{
	zassert_equal(-EINVAL, zbook_buzzer_beep_repeated(0));
	zassert_equal(0, fake_pwm_set_cycles_fake.call_count);
}

ZTEST(zbook_buzzer, test_16_beep_preset_lengths)
{
	zassert_ok(zbook_buzzer_beep(ZBOOK_BUZZER_BEEP_DURATION_SHORT));
	zassert_ok(zbook_buzzer_beep(ZBOOK_BUZZER_BEEP_DURATION_NORMAL));
	zassert_ok(zbook_buzzer_beep(ZBOOK_BUZZER_BEEP_DURATION_LONG));
}

ZTEST(zbook_buzzer, test_17_beep_invalid_args)
{
	zassert_equal(-EINVAL, zbook_buzzer_beep((enum zbook_buzzer_beep_duration)99));
	zassert_equal(0, fake_pwm_set_cycles_fake.call_count);
}

ZTEST(zbook_buzzer, test_18_beep_full_calls_multiple_times)
{
	zassert_ok(zbook_buzzer_beep_full(5, 4));

	zassert_equal(8, fake_pwm_set_cycles_fake.call_count);
}

ZTEST(zbook_buzzer, test_19_beep_full_invalid_args)
{
	zassert_equal(-EINVAL, zbook_buzzer_beep_full(10, 0));
	zassert_equal(0, fake_pwm_set_cycles_fake.call_count);
}
