/*******************************************************************
 * @file main.c
 *
 * @brief Unit tests for the Zbook microphone sensor input.
 *
 * @copyright Copyright (c) 2026
 *
 *******************************************************************/

#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/adc/adc_emul.h>
#include <zephyr/ztest.h>

#include "sensors/zbook_microphone.h"

#define MICROPHONE_ADC_CTLR DT_NODELABEL(adc)
#define MICROPHONE_CHANNEL  0

static const struct device *adc_dev;

#if defined(MICROPHONE_TEST_SETUP_FAILURE)

static void *microphone_suite_setup(void)
{
	adc_dev = DEVICE_DT_GET(MICROPHONE_ADC_CTLR);

	zassert_ok(device_init(adc_dev), "failed to bring up ADC emulator device");
	zassert_true(device_is_ready(adc_dev), "ADC emulator device not ready");

	return NULL;
}

ZTEST_SUITE(zbook_microphone, NULL, microphone_suite_setup, NULL, NULL, NULL);

ZTEST(zbook_microphone, test_init_reports_channel_setup_failure)
{
	zassert_equal(zbook_microphone_init(), -ENOTSUP,
		      "expected adc_channel_setup()'s failure to propagate");
}

#elif defined(MICROPHONE_TEST_READ_FAILURE)

static void *microphone_suite_setup(void)
{
	adc_dev = DEVICE_DT_GET(MICROPHONE_ADC_CTLR);

	zassert_ok(device_init(adc_dev), "failed to bring up ADC emulator device");
	zassert_true(device_is_ready(adc_dev), "ADC emulator device not ready");

	return NULL;
}

ZTEST_SUITE(zbook_microphone, NULL, microphone_suite_setup, NULL, NULL, NULL);

ZTEST(zbook_microphone, test_read_reports_adc_read_failure)
{
	uint16_t value = 123;

	zassert_not_equal(zbook_microphone_read(&value), 0,
			   "expected adc_read()'s failure to propagate");
}

#else

ZTEST_SUITE(zbook_hw_errors, NULL, NULL, NULL, NULL, NULL);

ZTEST(zbook_hw_errors, test_init_fails_while_device_not_ready)
{
	const struct device *dev = DEVICE_DT_GET(MICROPHONE_ADC_CTLR);

	zassert_false(device_is_ready(dev), "test setup: ADC device already ready");

	zassert_equal(zbook_microphone_init(), -ENODEV);

	zassert_ok(device_init(dev), "failed to bring up ADC emulator device for later tests");
	zassert_true(device_is_ready(dev));
}

static void *microphone_suite_setup(void)
{
	adc_dev = DEVICE_DT_GET(MICROPHONE_ADC_CTLR);
	zassert_true(device_is_ready(adc_dev), "ADC emulator device not ready");

	zassert_ok(zbook_microphone_init(), "zbook_microphone_init() failed");

	return NULL;
}

ZTEST_SUITE(zbook_microphone, NULL, microphone_suite_setup, NULL, NULL, NULL);

ZTEST(zbook_microphone, test_read_rejects_null_pointer)
{
	zassert_equal(zbook_microphone_read(NULL), -EINVAL);
}

static const uint32_t microphone_raw_cases[] = {0, 1, 2048, 4094, 4095};

ZTEST(zbook_microphone, test_read_returns_raw_sample)
{
	for (size_t i = 0; i < ARRAY_SIZE(microphone_raw_cases); i++) {
		uint16_t value = 0xffff;

		zassert_ok(adc_emul_const_raw_value_set(adc_dev, MICROPHONE_CHANNEL,
							 microphone_raw_cases[i]));
		zassert_ok(zbook_microphone_read(&value));
		zassert_equal(value, microphone_raw_cases[i], "raw=%u: got %u",
			      microphone_raw_cases[i], value);
	}
}

#endif /* MICROPHONE_TEST_SETUP_FAILURE / MICROPHONE_TEST_READ_FAILURE */
