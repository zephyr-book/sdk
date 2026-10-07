/************************************************************************
 * @file main.c
 *
 * @brief Unit tests for the Zbook light sensor input.
 * @author João Matheus Nascimento Dias (joao.dias@edge.ufal.br)
 * @version 0.1
 * @date 17/09/2026
 *
 * @copyright Centro de Inovação EDGE 2026. Todos os direitos reservados.
 *
 ***********************************************************************/

#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/adc/adc_emul.h>
#include <zephyr/ztest.h>

#include "sensors/zbook_ldr.h"

#define LDR_ADC_CTLR DT_NODELABEL(adc)
#define LDR_CHANNEL  0
#define LDR_MAX_RAW  4095 /* (1 << 12) - 1, zephyr,resolution of ldr_adc in app.overlay */

static const struct device *adc_dev;

#if defined(LDR_TEST_SETUP_FAILURE)

static void *ldr_suite_setup(void)
{
	adc_dev = DEVICE_DT_GET(LDR_ADC_CTLR);
	zassert_true(device_is_ready(adc_dev), "ADC emulator device not ready");

	return NULL;
}

ZTEST_SUITE(zbook_ldr, NULL, ldr_suite_setup, NULL, NULL, NULL);

ZTEST(zbook_ldr, test_init_reports_channel_setup_failure)
{
	zassert_equal(zbook_ldr_init(), -ENOTSUP,
		      "expected adc_channel_setup()'s failure to propagate");
}

#elif defined(LDR_TEST_READ_FAILURE)

static void *ldr_suite_setup(void)
{
	adc_dev = DEVICE_DT_GET(LDR_ADC_CTLR);
	zassert_true(device_is_ready(adc_dev), "ADC emulator device not ready");

	return NULL;
}

ZTEST_SUITE(zbook_ldr, NULL, ldr_suite_setup, NULL, NULL, NULL);

ZTEST(zbook_ldr, test_read_reports_adc_read_failure)
{
	uint16_t value = 123;

	/* zbook_ldr_read() doesn't require zbook_ldr_init() to have run first;
	 * adc_read() rejects the out-of-range channel on its own.
	 */
	zassert_not_equal(zbook_ldr_read(&value), 0, "expected adc_read()'s failure to propagate");
}

#else

static void *ldr_suite_setup(void)
{
	adc_dev = DEVICE_DT_GET(LDR_ADC_CTLR);
	zassert_true(device_is_ready(adc_dev), "ADC emulator device not ready");

	zassert_ok(zbook_ldr_init(), "zbook_ldr_init() failed");

	return NULL;
}

ZTEST_SUITE(zbook_ldr, NULL, ldr_suite_setup, NULL, NULL, NULL);

ZTEST(zbook_ldr, test_read_rejects_null_pointer)
{
	zassert_equal(zbook_ldr_read(NULL), -EINVAL);
}

ZTEST(zbook_ldr, test_read_zero_raw_reports_error)
{
	uint16_t value = 123;

	zassert_ok(adc_emul_const_raw_value_set(adc_dev, LDR_CHANNEL, 0));
	zassert_equal(zbook_ldr_read(&value), -EIO);
	zassert_equal(value, 0);
}

/* Hand-computed (independent of zbook_ldr.c's formula): raw * 100 / 4095. */
static const struct {
	uint32_t raw;
	uint16_t expect_percent;
} ldr_percent_cases[] = {
	{.raw = 1, .expect_percent = 0},      /* nonzero sample, 0% (vs. the raw=0 error case) */
	{.raw = 819, .expect_percent = 20},   /* exact: 819 * 100 / 4095 = 20 */
	{.raw = 2048, .expect_percent = 50},  /* just past half, floors to 50 */
	{.raw = 3276, .expect_percent = 80},  /* exact: 3276 * 100 / 4095 = 80 */
	{.raw = 4094, .expect_percent = 99},  /* just below full scale */
	{.raw = 4095, .expect_percent = 100}, /* full scale */
};

ZTEST(zbook_ldr, test_read_percent_table)
{
	for (size_t i = 0; i < ARRAY_SIZE(ldr_percent_cases); i++) {
		uint16_t value = 0xffff;

		zassert_ok(adc_emul_const_raw_value_set(adc_dev, LDR_CHANNEL,
							ldr_percent_cases[i].raw));
		zassert_ok(zbook_ldr_read(&value));
		zassert_equal(value, ldr_percent_cases[i].expect_percent,
			      "raw=%u: got %u%%, want %u%%", ldr_percent_cases[i].raw, value,
			      ldr_percent_cases[i].expect_percent);
	}
}

#endif /* LDR_TEST_SETUP_FAILURE / LDR_TEST_READ_FAILURE */
