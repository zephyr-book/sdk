/*******************************************************************
 * @file main.c
 *
 * @brief Real-hardware bring-up sample for the GPIO function of the zbook
 * header pins: exposes in/out/write/read/watch/release as shell commands,
 * addressed by IO name ("io01"/"io39"/"io45"/"io46").
 *
 * @author José Félix de Oliveira Neto (josefelix.neto@edge.ufal.br)
 * @version 0.1
 * @date 06/11/26
 *
 * @copyright Copyright (c) 2026
 *
 *******************************************************************/

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include "io/zbook_pin.h"

static struct {
	const char *name;
	enum zbook_pin_header pin;
	struct zbook_pin *handle;
} ios[] = {
	{"io01", ZBOOK_PIN_HEADER_IO_01, NULL},
	{"io39", ZBOOK_PIN_HEADER_IO_39, NULL},
	{"io45", ZBOOK_PIN_HEADER_IO_45, NULL},
	{"io46", ZBOOK_PIN_HEADER_IO_46, NULL},
};

static const char *const pull_names[] = {
	[ZBOOK_PIN_HEADER_PULL_NONE] = "none",
	[ZBOOK_PIN_HEADER_PULL_UP] = "up",
	[ZBOOK_PIN_HEADER_PULL_DOWN] = "down",
};

static const char *const state_names[] = {
	[ZBOOK_PIN_HEADER_STATE_LOW] = "low",
	[ZBOOK_PIN_HEADER_STATE_HIGH] = "high",
	[ZBOOK_PIN_HEADER_STATE_OPEN_DRAIN] = "od",
};

static const char *const edge_names[] = {
	[ZBOOK_PIN_HEADER_INTERRUPT_RISING] = "rising",
	[ZBOOK_PIN_HEADER_INTERRUPT_FALLING] = "falling",
	[ZBOOK_PIN_HEADER_INTERRUPT_BOTH] = "both",
};

static const char *const function_names[] = {
	[ZBOOK_PIN_HEADER_FUNCTION_GPIO] = "gpio",
	[ZBOOK_PIN_HEADER_FUNCTION_UART] = "uart",
	[ZBOOK_PIN_HEADER_FUNCTION_PWM] = "pwm",
	[ZBOOK_PIN_HEADER_FUNCTION_ADC] = "adc",
};

static int parse_name(const char *const *names, size_t count, const char *arg)
{
	for (size_t i = 0; i < count; i++) {
		if (names[i] != NULL && strcmp(names[i], arg) == 0) {
			return (int)i;
		}
	}

	return -EINVAL;
}

static int parse_io(const struct shell *sh, const char *arg)
{
	for (size_t i = 0; i < ARRAY_SIZE(ios); i++) {
		if (strcmp(ios[i].name, arg) == 0) {
			return (int)i;
		}
	}

	shell_error(sh, "unknown IO \"%s\" -- try io01/io39/io45/io46", arg);

	return -EINVAL;
}

static int parse_pull(const struct shell *sh, size_t argc, char **argv, size_t idx)
{
	if (argc <= idx) {
		return ZBOOK_PIN_HEADER_PULL_NONE;
	}

	int pull = parse_name(pull_names, ARRAY_SIZE(pull_names), argv[idx]);

	if (pull < 0) {
		shell_error(sh, "pull must be none/up/down");
	}

	return pull;
}

static int apply(const struct shell *sh, int io, const struct zbook_pin_header_config *cfg)
{
	int ret = zbook_pin_set_function(ios[io].pin, cfg, &ios[io].handle);

	if (ret != 0) {
		ios[io].handle = NULL;
		shell_error(sh, "zbook_pin_set_function failed: %d", ret);
	}

	return ret;
}

static int configured(const struct shell *sh, int io)
{
	if (ios[io].handle == NULL) {
		shell_error(sh, "\"%s\" not configured -- run 'gpio in' or 'gpio out' first",
			    ios[io].name);
		return -ENODEV;
	}

	return 0;
}

static int cmd_gpio_in(const struct shell *sh, size_t argc, char **argv)
{
	int io = parse_io(sh, argv[1]);
	int pull = parse_pull(sh, argc, argv, 2);

	if (io < 0 || pull < 0) {
		return -EINVAL;
	}

	struct zbook_pin_header_config cfg = ZBOOK_PIN_CFG_GPIO_IN(pull);
	int ret = apply(sh, io, &cfg);

	if (ret == 0) {
		shell_print(sh, "\"%s\" input, pull %s", ios[io].name, pull_names[pull]);
	}

	return ret;
}

static int cmd_gpio_out(const struct shell *sh, size_t argc, char **argv)
{
	int io = parse_io(sh, argv[1]);
	int state = ZBOOK_PIN_HEADER_STATE_LOW;

	if (io < 0) {
		return -EINVAL;
	}

	if (argc > 2) {
		state = parse_name(state_names, ARRAY_SIZE(state_names), argv[2]);
		if (state < 0) {
			shell_error(sh, "state must be low/high/od");
			return -EINVAL;
		}
	}

	struct zbook_pin_header_config cfg = ZBOOK_PIN_CFG_GPIO_OUT(state);
	int ret = apply(sh, io, &cfg);

	if (ret == 0) {
		shell_print(sh, "\"%s\" output, %s", ios[io].name, state_names[state]);
	}

	return ret;
}

static int cmd_gpio_write(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	int io = parse_io(sh, argv[1]);

	if (io < 0 || configured(sh, io) != 0) {
		return -EINVAL;
	}

	uint8_t state;

	if (strcmp(argv[2], "0") == 0) {
		state = ZBOOK_PIN_HEADER_STATE_LOW;
	} else if (strcmp(argv[2], "1") == 0) {
		state = ZBOOK_PIN_HEADER_STATE_HIGH;
	} else {
		shell_error(sh, "usage: gpio write <io> <0|1>");
		return -EINVAL;
	}

	int ret = zbook_pin_write(ios[io].handle, &state, 1);

	if (ret != 0) {
		shell_error(sh, "zbook_pin_write failed: %d", ret);
		return ret;
	}

	shell_print(sh, "\"%s\" set to %u", ios[io].name, state);

	return 0;
}

static int cmd_gpio_read(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	int io = parse_io(sh, argv[1]);

	if (io < 0 || configured(sh, io) != 0) {
		return -EINVAL;
	}

	uint8_t level;
	int ret = zbook_pin_read(ios[io].handle, &level, 1);

	if (ret != 0) {
		shell_error(sh, "zbook_pin_read failed: %d", ret);
		return ret;
	}

	shell_print(sh, "\"%s\": %u", ios[io].name, level);

	return 0;
}

static void watch_cb(enum zbook_pin_header pin, void *user_data)
{
	int io = POINTER_TO_INT(user_data);
	uint8_t level;
	int ret = zbook_pin_read(ios[io].handle, &level, 1);

	if (ret != 0) {
		printk("\"%s\" (GPIO%u) edge, read failed: %d\n", ios[io].name, pin, ret);
		return;
	}

	printk("\"%s\" (GPIO%u) edge, level %u\n", ios[io].name, pin, level);
}


static int cmd_gpio_watch(const struct shell *sh, size_t argc, char **argv)
{
	int io = parse_io(sh, argv[1]);
	int edge = parse_name(edge_names, ARRAY_SIZE(edge_names), argv[2]);
	int pull = parse_pull(sh, argc, argv, 3);

	if (edge < 0) {
		shell_error(sh, "edge must be rising/falling/both");
	}

	if (io < 0 || edge < 0 || pull < 0) {
		return -EINVAL;
	}

	struct zbook_pin_header_config cfg =
		ZBOOK_PIN_CFG_GPIO_IN_IRQ(pull, edge, watch_cb, INT_TO_POINTER(io));
	int ret = apply(sh, io, &cfg);

	if (ret == 0) {
		shell_print(sh, "watching \"%s\" for %s edges, pull %s", ios[io].name,
			    edge_names[edge], pull_names[pull]);
	}

	return ret;
}

static int cmd_gpio_release(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	int io = parse_io(sh, argv[1]);

	if (io < 0 || configured(sh, io) != 0) {
		return -EINVAL;
	}

	int ret = zbook_pin_release(ios[io].handle);

	if (ret != 0) {
		shell_error(sh, "zbook_pin_release failed: %d", ret);
		return ret;
	}

	ios[io].handle = NULL;
	shell_print(sh, "\"%s\" released", ios[io].name);

	return 0;
}

static int cmd_gpio_status(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	for (size_t i = 0; i < ARRAY_SIZE(ios); i++) {
		enum zbook_pin_header_function function;
		int ret = zbook_pin_get_function(ios[i].pin, &function);

		shell_print(sh, "%s: %s", ios[i].name,
			    ret == 0 ? function_names[function] : "unconfigured");
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_gpio,
	SHELL_CMD_ARG(in, NULL, "Configure an IO as input.\nUsage: gpio in <io> [none|up|down]",
		      cmd_gpio_in, 2, 1),
	SHELL_CMD_ARG(out, NULL, "Configure an IO as output.\nUsage: gpio out <io> [low|high|od]",
		      cmd_gpio_out, 2, 1),
	SHELL_CMD_ARG(write, NULL, "Drive an output IO.\nUsage: gpio write <io> <0|1>",
		      cmd_gpio_write, 3, 0),
	SHELL_CMD_ARG(read, NULL, "Read an IO.\nUsage: gpio read <io>", cmd_gpio_read, 2, 0),
	SHELL_CMD_ARG(watch, NULL,
		      "Configure an IO as input and print its edges.\n"
		      "Usage: gpio watch <io> <rising|falling|both> [none|up|down]",
		      cmd_gpio_watch, 3, 1),
	SHELL_CMD_ARG(release, NULL, "Release an IO.\nUsage: gpio release <io>", cmd_gpio_release,
		      2, 0),
	SHELL_CMD_ARG(status, NULL, "Show the function of every IO.\nUsage: gpio status",
		      cmd_gpio_status, 1, 0),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(gpio, &sub_gpio, "ZBook header pin GPIO commands.", NULL);

int main(void)
{
	printk("ZBook GPIO shell sample ready. Try: 'gpio out io01 high', then 'gpio' to see "
	       "available commands.\n");

	return 0;
}
