/************************************************************************
 * @file zbook_ir_shell.c
 *
 * @brief Shell commands for the Zbook IR I/O interface, to drive the IR
 *        emitter, read the IR receiver and log its events manually from a
 *        serial terminal.
 * @author Matheus Macário dos Santos (matheus.macario@edge.ufal.br)
 * @version 0.1
 * @date 10/11/2026
 *
 * @copyright Centro de Inovação EDGE 2026. Todos os direitos reservados.
 *
 ***********************************************************************/

#include <stdint.h>
#include <string.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/util.h>

#include "io/zbook_ir.h"

static const struct {
	const char *name;
	enum zbook_ir_evt evt;
} evt_masks[] = {
	{"detected", ZBOOK_IR_EVT_DETECTED},
	{"lost", ZBOOK_IR_EVT_LOST},
	{"both", ZBOOK_IR_EVT_BOTH},
};

static int parse_evt_mask(const struct shell *sh, const char *arg, enum zbook_ir_evt *evt)
{
	for (size_t i = 0; i < ARRAY_SIZE(evt_masks); i++) {
		if (strcmp(arg, evt_masks[i].name) == 0) {
			*evt = evt_masks[i].evt;
			return 0;
		}
	}

	shell_error(sh, "evento invalido: %s (use detected, lost ou both)", arg);
	return -EINVAL;
}

static int parse_duration(const struct shell *sh, const char *arg, uint32_t *duration_ms)
{
	int err = 0;
	unsigned long ms = shell_strtoul(arg, 10, &err);

	if (err != 0) {
		shell_error(sh, "duracao invalida: %s (use um numero em ms)", arg);
		return -EINVAL;
	}

	if (ms < 1) {
		shell_error(sh, "duracao invalida: %s (use 1-%d ms)", arg,
			    CONFIG_ZBOOK_INTERFACE_IO_IR_PULSE_MAX_MS);
		return -EINVAL;
	}

	if (ms > CONFIG_ZBOOK_INTERFACE_IO_IR_PULSE_MAX_MS) {
		shell_error(sh, "duracao invalida: %s (use 1-%d ms)", arg,
			    CONFIG_ZBOOK_INTERFACE_IO_IR_PULSE_MAX_MS);
		return -EINVAL;
	}

	*duration_ms = (uint32_t)ms;
	return 0;
}

static const char *evt_name(enum zbook_ir_evt evt)
{
	switch (evt) {
	case ZBOOK_IR_EVT_DETECTED:
		return "detected";
	case ZBOOK_IR_EVT_LOST:
		return "lost";
	default:
		return "unknown";
	}
}

static void shell_ir_cb(enum zbook_ir_evt evt, void *user_data)
{
	const struct shell *sh = user_data;

	shell_print(sh, "IR %s", evt_name(evt));
}

static int cmd_ir_on(const struct shell *sh, size_t argc, char **argv)
{
	int ret = zbook_ir_emitter_on();

	if (ret < 0) {
		shell_error(sh, "zbook_ir_emitter_on falhou (%d)", ret);
		return ret;
	}

	return 0;
}

static int cmd_ir_off(const struct shell *sh, size_t argc, char **argv)
{
	int ret = zbook_ir_emitter_off();

	if (ret < 0) {
		shell_error(sh, "zbook_ir_emitter_off falhou (%d)", ret);
		return ret;
	}

	return 0;
}

static int cmd_ir_toggle(const struct shell *sh, size_t argc, char **argv)
{
	int ret = zbook_ir_emitter_toggle();

	if (ret < 0) {
		shell_error(sh, "zbook_ir_emitter_toggle falhou (%d)", ret);
		return ret;
	}

	return 0;
}

static int cmd_ir_pulse(const struct shell *sh, size_t argc, char **argv)
{
	uint32_t duration_ms;
	int ret;

	ret = parse_duration(sh, argv[1], &duration_ms);
	if (ret < 0) {
		return ret;
	}

	ret = zbook_ir_emitter_pulse(duration_ms);
	if (ret < 0) {
		shell_error(sh, "zbook_ir_emitter_pulse falhou (%d)", ret);
		return ret;
	}

	return 0;
}

static int cmd_ir_state(const struct shell *sh, size_t argc, char **argv)
{
	int ret = zbook_ir_emitter_get();

	if (ret < 0) {
		shell_error(sh, "zbook_ir_emitter_get falhou (%d)", ret);
		return ret;
	}

	shell_print(sh, "emissor IR %s", ret ? "ligado" : "desligado");

	return 0;
}

static int cmd_ir_read(const struct shell *sh, size_t argc, char **argv)
{
	int ret = zbook_ir_receiver_read();

	if (ret < 0) {
		shell_error(sh, "zbook_ir_receiver_read falhou (%d)", ret);
		return ret;
	}

	shell_print(sh, "receptor IR %s", ret ? "detectando" : "sem IR");

	return 0;
}

static int cmd_ir_reg(const struct shell *sh, size_t argc, char **argv)
{
	enum zbook_ir_evt evt;
	int ret;

	ret = parse_evt_mask(sh, argv[1], &evt);
	if (ret < 0) {
		return ret;
	}

	ret = zbook_ir_reg_cb(evt, shell_ir_cb, (void *)sh);
	if (ret < 0) {
		shell_error(sh, "zbook_ir_reg_cb falhou (%d)", ret);
		return ret;
	}

	return 0;
}

static int cmd_ir_rm(const struct shell *sh, size_t argc, char **argv)
{
	int ret = zbook_ir_rm_cb();

	if (ret < 0) {
		shell_error(sh, "zbook_ir_rm_cb falhou (%d)", ret);
		return ret;
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_ir, SHELL_CMD_ARG(on, NULL, "ir on", cmd_ir_on, 1, 0),
	SHELL_CMD_ARG(off, NULL, "ir off", cmd_ir_off, 1, 0),
	SHELL_CMD_ARG(toggle, NULL, "ir toggle", cmd_ir_toggle, 1, 0),
	SHELL_CMD_ARG(pulse, NULL, "ir pulse <ms>", cmd_ir_pulse, 2, 0),
	SHELL_CMD_ARG(state, NULL, "ir state (estado do emissor)", cmd_ir_state, 1, 0),
	SHELL_CMD_ARG(read, NULL, "ir read (nivel do receptor)", cmd_ir_read, 1, 0),
	SHELL_CMD_ARG(reg, NULL, "ir reg <detected|lost|both> (mostra eventos do receptor)",
		      cmd_ir_reg, 2, 0),
	SHELL_CMD_ARG(rm, NULL, "ir rm (para de mostrar eventos)", cmd_ir_rm, 1, 0),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(ir, &sub_ir, "Zbook IR I/O commands", NULL);
