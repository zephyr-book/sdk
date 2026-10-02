/************************************************************************
 * @file main.c
 *
 * @brief Unit tests for the Zbook potentiometer sensor input.
 * @author João Matheus Nascimento Dias (joao.dias@edge.ufal.br)
 * @version 0.2
 * @date 18/09/2026
 *
 * @copyright Centro de Inovação EDGE 2026. Todos os direitos reservados.
 *
 ***********************************************************************/

#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/adc/adc_emul.h>
#include <zephyr/ztest.h>

#include "sensors/zbook_potentiometer.h"

#define POTENTIOMETER_ADC_CTLR DT_NODELABEL(adc)
#define POTENTIOMETER_CHANNEL  0
#define POTENTIOMETER_MAX_RAW                                                                      \
	4095 /* (1 << 12) - 1, POTENTIOMETER_RESOLUTION in zbook_potentiometer.c */

static const struct device *adc_dev;

#if defined(POTENTIOMETER_TEST_SETUP_FAILURE)

static void *potentiometer_suite_setup(void)
{
	adc_dev = DEVICE_DT_GET(POTENTIOMETER_ADC_CTLR);
	zassert_true(device_is_ready(adc_dev), "ADC emulator device not ready");

	return NULL;
}

ZTEST_SUITE(zbook_potentiometer, NULL, potentiometer_suite_setup, NULL, NULL, NULL);

ZTEST(zbook_potentiometer, test_init_reports_channel_setup_failure)
{
	zassert_equal(zbook_potentiometer_init(), -ENOTSUP,
		      "expected adc_channel_setup()'s failure to propagate");
}

#elif defined(POTENTIOMETER_TEST_READ_FAILURE)

static void *potentiometer_suite_setup(void)
{
	adc_dev = DEVICE_DT_GET(POTENTIOMETER_ADC_CTLR);
	zassert_true(device_is_ready(adc_dev), "ADC emulator device not ready");

	return NULL;
}

ZTEST_SUITE(zbook_potentiometer, NULL, potentiometer_suite_setup, NULL, NULL, NULL);

ZTEST(zbook_potentiometer, test_read_reports_adc_read_failure)
{
	uint16_t value = 123;

	/* zbook_potentiometer_read() doesn't require zbook_potentiometer_init() to have run
	 * first; adc_read() rejects the out-of-range channel on its own.
	 */
	zassert_not_equal(zbook_potentiometer_read(&value), 0,
			  "expected adc_read()'s failure to propagate");
}

#else

static void *potentiometer_suite_setup(void)
{
	adc_dev = DEVICE_DT_GET(POTENTIOMETER_ADC_CTLR);
	zassert_true(device_is_ready(adc_dev), "ADC emulator device not ready");

	zassert_ok(zbook_potentiometer_init(), "zbook_potentiometer_init() failed");

	return NULL;
}

ZTEST_SUITE(zbook_potentiometer, NULL, potentiometer_suite_setup, NULL, NULL, NULL);

ZTEST(zbook_potentiometer, test_read_rejects_null_pointer)
{
	zassert_equal(zbook_potentiometer_read(NULL), -EINVAL);
}

ZTEST(zbook_potentiometer, test_read_zero_raw_reports_error)
{
	uint16_t value = 123;

	zassert_ok(adc_emul_const_raw_value_set(adc_dev, POTENTIOMETER_CHANNEL, 0));
	zassert_equal(zbook_potentiometer_read(&value), -EIO);
	zassert_equal(value, 0);
}

/* Hand-computed (independent of zbook_potentiometer.c's formula): raw * 100 / 4095. */
static const struct {
	uint32_t raw;
	uint16_t expect_percent;
} potentiometer_percent_cases[] = {
	{.raw = 1, .expect_percent = 0},      /* nonzero sample, 0% (vs. the raw=0 error case) */
	{.raw = 819, .expect_percent = 20},   /* exact: 819 * 100 / 4095 = 20 */
	{.raw = 2048, .expect_percent = 50},  /* just past half, floors to 50 */
	{.raw = 3276, .expect_percent = 80},  /* exact: 3276 * 100 / 4095 = 80 */
	{.raw = 4094, .expect_percent = 99},  /* just below full scale */
	{.raw = 4095, .expect_percent = 100}, /* full scale */
};

ZTEST(zbook_potentiometer, test_read_percent_table)
{
	for (size_t i = 0; i < ARRAY_SIZE(potentiometer_percent_cases); i++) {
		uint16_t value = 0xffff;

		zassert_ok(adc_emul_const_raw_value_set(adc_dev, POTENTIOMETER_CHANNEL,
							potentiometer_percent_cases[i].raw));
		zassert_ok(zbook_potentiometer_read(&value));
		zassert_equal(value, potentiometer_percent_cases[i].expect_percent,
			      "raw=%u: got %u%%, want %u%%", potentiometer_percent_cases[i].raw,
			      value, potentiometer_percent_cases[i].expect_percent);
	}
}

#endif /* POTENTIOMETER_TEST_SETUP_FAILURE / POTENTIOMETER_TEST_READ_FAILURE */
