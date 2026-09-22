/*******************************************************************
 * @file main.c
 *
 * @brief Real-hardware bring-up sample for zbook generic PWM channels:
 * exposes init/start/stop/set/get as shell commands, addressed by IO name
 * ("io01"/"io39"/"io45"/"io46"), for manual testing over the console.
 *
 * @author José Félix de Oliveira Neto (josefelix.neto@edge.ufal.br)
 * @version 0.1
 * @date 16/09/26
 *
 * @copyright Copyright (c) 2026
 *
 *******************************************************************/

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_string_conv.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include "actuators/zbook_pwm.h"

static const struct {
	const char *name;
	enum zbook_pwm_io io;
} io_table[] = {
	{"io01", ZBOOK_PWM_IO01},
	{"io39", ZBOOK_PWM_IO39},
	{"io45", ZBOOK_PWM_IO45},
	{"io46", ZBOOK_PWM_IO46},
};

static int parse_io(const char *name, enum zbook_pwm_io *io)
{
	for (size_t i = 0; i < ARRAY_SIZE(io_table); i++) {
		if (strcmp(io_table[i].name, name) == 0) {
			*io = io_table[i].io;
			return 0;
		}
	}

	return -EINVAL;
}

static int cmd_pwm_init(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	enum zbook_pwm_io io;

	if (parse_io(argv[1], &io) != 0) {
		shell_error(sh, "unknown IO \"%s\" -- try io01/io39/io45/io46", argv[1]);
		return -EINVAL;
	}

	int ret = zbook_pwm_init(io);

	if (ret != 0) {
		shell_error(sh, "zbook_pwm_init failed: %d", ret);
		return ret;
	}

	shell_print(sh, "\"%s\" initialized", argv[1]);

	return 0;
}

static int cmd_pwm_start(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	enum zbook_pwm_io io;

	if (parse_io(argv[1], &io) != 0) {
		shell_error(sh, "unknown IO \"%s\" -- try io01/io39/io45/io46", argv[1]);
		return -EINVAL;
	}

	int ret = zbook_pwm_start(io);

	if (ret != 0) {
		shell_error(sh, "zbook_pwm_start failed: %d", ret);
		return ret;
	}

	shell_print(sh, "\"%s\" started", argv[1]);

	return 0;
}

static int cmd_pwm_stop(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	enum zbook_pwm_io io;

	if (parse_io(argv[1], &io) != 0) {
		shell_error(sh, "unknown IO \"%s\" -- try io01/io39/io45/io46", argv[1]);
		return -EINVAL;
	}

	int ret = zbook_pwm_stop(io);

	if (ret != 0) {
		shell_error(sh, "zbook_pwm_stop failed: %d", ret);
		return ret;
	}

	shell_print(sh, "\"%s\" stopped", argv[1]);

	return 0;
}

static int cmd_pwm_set(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	enum zbook_pwm_io io;

	if (parse_io(argv[1], &io) != 0) {
		shell_error(sh, "unknown IO \"%s\" -- try io01/io39/io45/io46", argv[1]);
		return -EINVAL;
	}

	int err = 0;
	struct zbook_pwm_cfg cfg = {
		.frequency_hz = (uint32_t)shell_strtoul(argv[2], 10, &err),
		.duty_cycle_percent = (uint8_t)shell_strtoul(argv[3], 10, &err),
	};

	if (strcmp(argv[4], "normal") == 0) {
		cfg.polarity = ZBOOK_PWM_POLARITY_NORMAL;
	} else if (strcmp(argv[4], "inverted") == 0) {
		cfg.polarity = ZBOOK_PWM_POLARITY_INVERTED;
	} else {
		err = -EINVAL;
	}

	if (err != 0) {
		shell_error(sh, "usage: pwm set <io> <freq_hz> <duty 0-100> <normal|inverted>");
		return -EINVAL;
	}

	int ret = zbook_pwm_set_cfg(io, &cfg);

	if (ret != 0) {
		shell_error(sh, "zbook_pwm_set_cfg failed: %d", ret);
		return ret;
	}

	shell_print(sh, "\"%s\" set to %u Hz, %u%% duty, %s", argv[1], cfg.frequency_hz,
		    cfg.duty_cycle_percent, argv[4]);

	return 0;
}

static int cmd_pwm_get(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	enum zbook_pwm_io io;

	if (parse_io(argv[1], &io) != 0) {
		shell_error(sh, "unknown IO \"%s\" -- try io01/io39/io45/io46", argv[1]);
		return -EINVAL;
	}

	struct zbook_pwm_cfg cfg;
	int ret = zbook_pwm_get_cfg(io, &cfg);

	if (ret != 0) {
		shell_error(sh, "zbook_pwm_get_cfg failed: %d", ret);
		return ret;
	}

	shell_print(sh, "\"%s\": %u Hz, %u%% duty, %s", argv[1], cfg.frequency_hz,
		    cfg.duty_cycle_percent,
		    cfg.polarity == ZBOOK_PWM_POLARITY_INVERTED ? "inverted" : "normal");

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_pwm,
	SHELL_CMD_ARG(init, NULL, "Initialize an IO.\nUsage: pwm init <io>", cmd_pwm_init, 2, 0),
	SHELL_CMD_ARG(start, NULL, "Start an IO.\nUsage: pwm start <io>", cmd_pwm_start, 2, 0),
	SHELL_CMD_ARG(stop, NULL, "Stop an IO.\nUsage: pwm stop <io>", cmd_pwm_stop, 2, 0),
	SHELL_CMD_ARG(set, NULL,
		      "Configure an IO.\n"
		      "Usage: pwm set <io> <freq_hz> <duty 0-100> <normal|inverted>",
		      cmd_pwm_set, 5, 0),
	SHELL_CMD_ARG(get, NULL, "Read back an IO's config.\nUsage: pwm get <io>", cmd_pwm_get, 2,
		      0),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(pwm, &sub_pwm, "ZBook generic PWM channel commands.", NULL);

int main(void)
{
	printk("ZBook PWM shell sample ready. Try: 'pwm init io01', then 'pwm' to see available "
	       "commands.\n");

	return 0;
}
