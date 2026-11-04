/*************************************************************************
 * @file test_pin.c
 *
 * @brief Unit tests of the Zbook header pin-function factory (zbook_pin.c).
 * @author João Matheus Nascimento Dias (joao.dias@edge.ufal.br)
 * @version 0.1
 * @date 20/10/2026
 *
 * @copyright Centro de Inovação EDGE 2026. Todos os direitos reservados.
 *
 ***********************************************************************/

#include "fake_backends.h"

#include "io/zbook_pin.h"

#include <errno.h>
#include <zephyr/ztest.h>

#define GPIO_FN ZBOOK_PIN_HEADER_FUNCTION_GPIO
#define UART_FN ZBOOK_PIN_HEADER_FUNCTION_UART
#define PWM_FN  ZBOOK_PIN_HEADER_FUNCTION_PWM
#define ADC_FN  ZBOOK_PIN_HEADER_FUNCTION_ADC

/* Pin without ADC and pin with ADC. */
#define PIN_PLAIN ZBOOK_PIN_HEADER_IO_01
#define PIN_ADC   ZBOOK_PIN_HEADER_IO_45

/* Not a header pin. */
#define PIN_INVALID ((enum zbook_pin_header)2)

static const enum zbook_pin_header all_pins[] = {
	ZBOOK_PIN_HEADER_IO_01,
	ZBOOK_PIN_HEADER_IO_39,
	ZBOOK_PIN_HEADER_IO_45,
	ZBOOK_PIN_HEADER_IO_46,
};

/* The pin table is static state: put every pin back to unconfigured. */
static void release_all(void)
{
	fake_reset();

	for (size_t i = 0; i < ARRAY_SIZE(all_pins); i++) {
		struct zbook_pin *pin;

		zassert_ok(zbook_pin_set_function(
			all_pins[i], &ZBOOK_PIN_CFG_GPIO_IN(ZBOOK_PIN_HEADER_PULL_NONE), &pin));
		zassert_ok(zbook_pin_release(pin));
	}

	fake_reset();
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);
	release_all();
}

ZTEST_SUITE(zbook_pin, NULL, NULL, before, NULL, NULL);

/* supports */

ZTEST(zbook_pin, test_supports_common_functions_on_every_pin)
{
	for (size_t i = 0; i < ARRAY_SIZE(all_pins); i++) {
		zassert_true(zbook_pin_supports(all_pins[i], GPIO_FN));
		zassert_true(zbook_pin_supports(all_pins[i], UART_FN));
		zassert_true(zbook_pin_supports(all_pins[i], PWM_FN));
	}
}

ZTEST(zbook_pin, test_supports_adc_only_on_adc_pins)
{
	zassert_false(zbook_pin_supports(ZBOOK_PIN_HEADER_IO_01, ADC_FN));
	zassert_false(zbook_pin_supports(ZBOOK_PIN_HEADER_IO_39, ADC_FN));
	zassert_true(zbook_pin_supports(ZBOOK_PIN_HEADER_IO_45, ADC_FN));
	zassert_true(zbook_pin_supports(ZBOOK_PIN_HEADER_IO_46, ADC_FN));
}

ZTEST(zbook_pin, test_supports_rejects_invalid_pin_and_function)
{
	zassert_false(zbook_pin_supports(PIN_INVALID, GPIO_FN));
	zassert_false(zbook_pin_supports(PIN_PLAIN, ZBOOK_PIN_HEADER_FUNCTION_AMOUNT));
}

/* set_function */

ZTEST(zbook_pin, test_set_function_rejects_invalid_arguments)
{
	struct zbook_pin *pin = NULL;

	zassert_equal(zbook_pin_set_function(PIN_INVALID, &ZBOOK_PIN_CFG_GPIO_OUT(0), &pin),
		      -EINVAL);
	zassert_equal(zbook_pin_set_function(PIN_PLAIN, NULL, &pin), -EINVAL);
	zassert_is_null(pin);
	zassert_equal(fake_backend(GPIO_FN)->init_calls, 0);
}

ZTEST(zbook_pin, test_set_function_rejects_unsupported_function)
{
	struct zbook_pin *pin = NULL;

	zassert_equal(zbook_pin_set_function(PIN_PLAIN, &ZBOOK_PIN_CFG_ADC(12), &pin), -ENOTSUP);
	zassert_is_null(pin);
	zassert_equal(fake_backend(ADC_FN)->init_calls, 0);
}

ZTEST(zbook_pin, test_set_function_configures_and_returns_handle)
{
	struct zbook_pin *pin = NULL;
	enum zbook_pin_header_function function;

	zassert_ok(zbook_pin_set_function(PIN_PLAIN, &ZBOOK_PIN_CFG_GPIO_OUT(0), &pin));
	zassert_not_null(pin);
	zassert_equal(fake_backend(GPIO_FN)->init_calls, 1);

	zassert_ok(zbook_pin_get_function(PIN_PLAIN, &function));
	zassert_equal(function, GPIO_FN);
}

ZTEST(zbook_pin, test_set_function_handle_is_optional)
{
	enum zbook_pin_header_function function;

	zassert_ok(zbook_pin_set_function(PIN_ADC, &ZBOOK_PIN_CFG_ADC(12), NULL));
	zassert_ok(zbook_pin_get_function(PIN_ADC, &function));
	zassert_equal(function, ADC_FN);
}

ZTEST(zbook_pin, test_set_function_releases_previous_function_first)
{
	struct zbook_pin *pin;

	zassert_ok(zbook_pin_set_function(PIN_PLAIN, &ZBOOK_PIN_CFG_GPIO_OUT(0), &pin));
	zassert_ok(zbook_pin_set_function(PIN_PLAIN, &ZBOOK_PIN_CFG_PWM(1000, 0), &pin));

	zassert_equal(fake_event_count(), 3);
	zassert_equal(fake_event(0)->function, GPIO_FN);
	zassert_equal(fake_event(0)->op, FAKE_OP_INIT);
	zassert_equal(fake_event(1)->function, GPIO_FN);
	zassert_equal(fake_event(1)->op, FAKE_OP_DEINIT);
	zassert_equal(fake_event(2)->function, PWM_FN);
	zassert_equal(fake_event(2)->op, FAKE_OP_INIT);
}

ZTEST(zbook_pin, test_set_function_same_function_reconfigures)
{
	struct zbook_pin *pin;

	zassert_ok(zbook_pin_set_function(PIN_PLAIN, &ZBOOK_PIN_CFG_GPIO_OUT(0), &pin));
	zassert_ok(zbook_pin_set_function(PIN_PLAIN, &ZBOOK_PIN_CFG_GPIO_OUT(1), &pin));

	zassert_equal(fake_backend(GPIO_FN)->init_calls, 2);
	zassert_equal(fake_backend(GPIO_FN)->deinit_calls, 1);
}

ZTEST(zbook_pin, test_set_function_init_failure_leaves_pin_unconfigured)
{
	struct zbook_pin *pin;
	struct zbook_pin *untouched = NULL;
	enum zbook_pin_header_function function;

	zassert_ok(zbook_pin_set_function(PIN_PLAIN, &ZBOOK_PIN_CFG_GPIO_OUT(0), &pin));

	fake_backend(PWM_FN)->init_ret = -EIO;
	zassert_equal(zbook_pin_set_function(PIN_PLAIN, &ZBOOK_PIN_CFG_PWM(1000, 0), &untouched),
		      -EIO);

	zassert_is_null(untouched);
	zassert_equal(fake_backend(GPIO_FN)->deinit_calls, 1, "old function must be released");
	zassert_equal(zbook_pin_get_function(PIN_PLAIN, &function), -ENODATA);
}

ZTEST(zbook_pin, test_set_function_accepts_uart)
{
	struct zbook_pin *pin;

	zassert_ok(zbook_pin_set_function(PIN_PLAIN, &ZBOOK_PIN_CFG_UART_TX(115200), &pin));
	zassert_equal(fake_backend(UART_FN)->init_calls, 1);
}

/* get_function */

ZTEST(zbook_pin, test_get_function_unconfigured_returns_enodata)
{
	enum zbook_pin_header_function function;

	zassert_equal(zbook_pin_get_function(PIN_PLAIN, &function), -ENODATA);
}

ZTEST(zbook_pin, test_get_function_rejects_invalid_arguments)
{
	enum zbook_pin_header_function function;

	zassert_equal(zbook_pin_get_function(PIN_INVALID, &function), -EINVAL);
	zassert_equal(zbook_pin_get_function(PIN_PLAIN, NULL), -EINVAL);
}

/* release */

ZTEST(zbook_pin, test_release_deinitializes_and_unconfigures)
{
	struct zbook_pin *pin;
	enum zbook_pin_header_function function;

	zassert_ok(zbook_pin_set_function(PIN_PLAIN, &ZBOOK_PIN_CFG_GPIO_OUT(0), &pin));
	zassert_ok(zbook_pin_release(pin));

	zassert_equal(fake_backend(GPIO_FN)->deinit_calls, 1);
	zassert_equal(zbook_pin_get_function(PIN_PLAIN, &function), -ENODATA);
}

ZTEST(zbook_pin, test_release_unconfigured_pin_is_noop)
{
	struct zbook_pin *pin;

	zassert_ok(zbook_pin_set_function(PIN_PLAIN, &ZBOOK_PIN_CFG_GPIO_OUT(0), &pin));
	zassert_ok(zbook_pin_release(pin));
	zassert_ok(zbook_pin_release(pin));

	zassert_equal(fake_backend(GPIO_FN)->deinit_calls, 1);
}

ZTEST(zbook_pin, test_release_rejects_invalid_handle)
{
	struct zbook_pin not_a_handle = {0};

	zassert_equal(zbook_pin_release(NULL), -EINVAL);
	zassert_equal(zbook_pin_release(&not_a_handle), -EINVAL);
}

/* write / read */

ZTEST(zbook_pin, test_write_reaches_gpio_backend)
{
	struct zbook_pin *pin;
	uint8_t state = ZBOOK_PIN_HEADER_STATE_HIGH;

	zassert_ok(zbook_pin_set_function(PIN_PLAIN, &ZBOOK_PIN_CFG_GPIO_OUT(0), &pin));
	zassert_ok(zbook_pin_write(pin, &state, sizeof(state)));

	zassert_equal(fake_backend(GPIO_FN)->write_calls, 1);
	zassert_equal(fake_backend(GPIO_FN)->last_len, 1);
	zassert_equal(fake_backend(GPIO_FN)->last_wr, ZBOOK_PIN_HEADER_STATE_HIGH);
}

ZTEST(zbook_pin, test_write_reaches_pwm_backend)
{
	struct zbook_pin *pin;
	uint8_t duty = 40;

	zassert_ok(zbook_pin_set_function(PIN_PLAIN, &ZBOOK_PIN_CFG_PWM(1000, 0), &pin));
	zassert_ok(zbook_pin_write(pin, &duty, sizeof(duty)));

	zassert_equal(fake_backend(PWM_FN)->write_calls, 1);
	zassert_equal(fake_backend(PWM_FN)->last_wr, 40);
}

ZTEST(zbook_pin, test_read_reaches_gpio_backend)
{
	struct zbook_pin *pin;
	uint8_t state = 0;

	fake_backend(GPIO_FN)->rd_val = ZBOOK_PIN_HEADER_STATE_HIGH;
	zassert_ok(zbook_pin_set_function(PIN_PLAIN,
					  &ZBOOK_PIN_CFG_GPIO_IN(ZBOOK_PIN_HEADER_PULL_UP), &pin));
	zassert_ok(zbook_pin_read(pin, &state, sizeof(state)));

	zassert_equal(state, ZBOOK_PIN_HEADER_STATE_HIGH);
	zassert_equal(fake_backend(GPIO_FN)->read_calls, 1);
}

ZTEST(zbook_pin, test_read_reaches_adc_backend)
{
	struct zbook_pin *pin;
	uint16_t raw = 0xFFFF;

	fake_backend(ADC_FN)->rd_val = 0x2A;
	zassert_ok(zbook_pin_set_function(PIN_ADC, &ZBOOK_PIN_CFG_ADC(12), &pin));
	zassert_ok(zbook_pin_read(pin, &raw, sizeof(raw)));

	zassert_equal(raw, 0x2A);
	zassert_equal(fake_backend(ADC_FN)->last_len, sizeof(raw));
}

ZTEST(zbook_pin, test_operation_missing_in_backend_returns_enotsup)
{
	struct zbook_pin *pin;
	uint8_t byte = 0;

	/* PWM has no read. */
	zassert_ok(zbook_pin_set_function(PIN_PLAIN, &ZBOOK_PIN_CFG_PWM(1000, 0), &pin));
	zassert_equal(zbook_pin_read(pin, &byte, sizeof(byte)), -ENOTSUP);

	/* ADC has no write. */
	zassert_ok(zbook_pin_set_function(PIN_ADC, &ZBOOK_PIN_CFG_ADC(12), &pin));
	zassert_equal(zbook_pin_write(pin, &byte, sizeof(byte)), -ENOTSUP);

	/* UART has neither yet. */
	zassert_ok(zbook_pin_set_function(PIN_PLAIN, &ZBOOK_PIN_CFG_UART_RX(115200), &pin));
	zassert_equal(zbook_pin_read(pin, &byte, sizeof(byte)), -ENOTSUP);
	zassert_equal(zbook_pin_write(pin, &byte, sizeof(byte)), -ENOTSUP);
}

ZTEST(zbook_pin, test_backend_error_is_propagated)
{
	struct zbook_pin *pin;
	uint8_t byte = 0;

	zassert_ok(zbook_pin_set_function(PIN_PLAIN, &ZBOOK_PIN_CFG_GPIO_OUT(0), &pin));

	fake_backend(GPIO_FN)->rw_ret = -EIO;
	zassert_equal(zbook_pin_write(pin, &byte, sizeof(byte)), -EIO);
	zassert_equal(zbook_pin_read(pin, &byte, sizeof(byte)), -EIO);
}

ZTEST(zbook_pin, test_read_write_reject_invalid_arguments)
{
	struct zbook_pin *pin;
	struct zbook_pin not_a_handle = {0};
	uint8_t byte = 0;

	zassert_ok(zbook_pin_set_function(PIN_PLAIN, &ZBOOK_PIN_CFG_GPIO_OUT(0), &pin));

	zassert_equal(zbook_pin_write(NULL, &byte, sizeof(byte)), -EINVAL);
	zassert_equal(zbook_pin_read(NULL, &byte, sizeof(byte)), -EINVAL);
	zassert_equal(zbook_pin_write(&not_a_handle, &byte, sizeof(byte)), -EINVAL);
	zassert_equal(zbook_pin_write(pin, NULL, 1), -EINVAL);
	zassert_equal(zbook_pin_read(pin, NULL, 1), -EINVAL);
	zassert_equal(fake_backend(GPIO_FN)->write_calls + fake_backend(GPIO_FN)->read_calls, 0);
}

ZTEST(zbook_pin, test_read_write_on_released_pin_return_einval)
{
	struct zbook_pin *pin;
	uint8_t byte = 0;

	zassert_ok(zbook_pin_set_function(PIN_PLAIN, &ZBOOK_PIN_CFG_GPIO_OUT(0), &pin));
	zassert_ok(zbook_pin_release(pin));

	zassert_equal(zbook_pin_write(pin, &byte, sizeof(byte)), -EINVAL);
	zassert_equal(zbook_pin_read(pin, &byte, sizeof(byte)), -EINVAL);
}
