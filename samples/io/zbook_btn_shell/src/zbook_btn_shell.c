/************************************************************************
 * @file zbook_btn_shell.c
 *
 * @brief Shell commands for the Zbook button I/O interface, to register
 *        and remove event callbacks manually from a serial terminal.
 * @author João Matheus Nascimento Dias (joao.dias@edge.ufal.br)
 *
 * @copyright Centro de Inovação EDGE 2026. Todos os direitos reservados.
 *
 ***********************************************************************/

#include <stdlib.h>
#include <string.h>
#include <zephyr/shell/shell.h>

#include "io/zbook_btn.h"

static int parse_btn(const struct shell *sh, const char *arg, enum zbook_btn *btn)
{
	if (strcmp(arg, "all") == 0) {
		*btn = ZBOOK_BTN_ALL;
		return 0;
	}

	char *end;
	long n = strtol(arg, &end, 10);

	if (*end != '\0' || n < 1 || n > 4) {
		shell_error(sh, "btn invalido: %s (use 1-4 ou all)", arg);
		return -EINVAL;
	}

	*btn = (enum zbook_btn)(n - 1);
	return 0;
}

static int parse_evt_mask(const struct shell *sh, const char *arg, enum zbook_btn_evt *evt)
{
	if (strcmp(arg, "pressed") == 0) {
		*evt = ZBOOK_BTN_EVT_PRESSED;
	} else if (strcmp(arg, "released") == 0) {
		*evt = ZBOOK_BTN_EVT_RELEASED;
	} else if (strcmp(arg, "long") == 0) {
		*evt = ZBOOK_BTN_EVT_LONG_PRESSED;
	} else if (strcmp(arg, "both") == 0) {
		*evt = ZBOOK_BTN_EVT_BOTH;
	} else if (strcmp(arg, "all") == 0) {
		*evt = ZBOOK_BTN_EVT_ALL;
	} else {
		shell_error(sh, "evento invalido: %s (use pressed, released, long, both ou all)",
			    arg);
		return -EINVAL;
	}

	return 0;
}

static const char *evt_name(enum zbook_btn_evt evt)
{
	switch (evt) {
	case ZBOOK_BTN_EVT_PRESSED:
		return "pressed";
	case ZBOOK_BTN_EVT_RELEASED:
		return "released";
	case ZBOOK_BTN_EVT_LONG_PRESSED:
		return "long-pressed";
	default:
		return "unknown";
	}
}

static void shell_btn_cb(enum zbook_btn btn, enum zbook_btn_evt evt, void *user_data)
{
	const struct shell *sh = user_data;

	shell_print(sh, "BTN%d %s", btn, evt_name(evt));
}

static int cmd_btn_reg(const struct shell *sh, size_t argc, char **argv)
{
	enum zbook_btn btn;
	enum zbook_btn_evt evt;
	int ret;

	ret = parse_btn(sh, argv[1], &btn);
	if (ret < 0) {
		return ret;
	}

	ret = parse_evt_mask(sh, argv[2], &evt);
	if (ret < 0) {
		return ret;
	}

	ret = zbook_btn_reg_cb(btn, evt, shell_btn_cb, (void *)sh);
	if (ret < 0) {
		shell_error(sh, "zbook_btn_reg_cb falhou (%d)", ret);
		return ret;
	}

	return 0;
}

static int cmd_btn_rm(const struct shell *sh, size_t argc, char **argv)
{
	enum zbook_btn btn;
	int ret;

	ret = parse_btn(sh, argv[1], &btn);
	if (ret < 0) {
		return ret;
	}

	ret = zbook_btn_rm_cb(btn);
	if (ret < 0) {
		shell_error(sh, "zbook_btn_rm_cb falhou (%d)", ret);
		return ret;
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_btn,
			       SHELL_CMD_ARG(reg, NULL,
					     "btn reg <1-4|all> <pressed|released|long|both|all>",
					     cmd_btn_reg, 3, 0),
			       SHELL_CMD_ARG(rm, NULL, "btn rm <1-4|all>", cmd_btn_rm, 2, 0),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(btn, &sub_btn, "Zbook button I/O commands", NULL);
