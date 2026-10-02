/************************************************************************
 * @file zbook_led_shell.c
 *
 * @brief Shell commands for the Zbook LED actuator, to drive it manually
 *        from a serial terminal.
 * @author João Matheus Nascimento Dias (joao.dias@edge.ufal.br)
 *
 * @copyright Centro de Inovação EDGE 2026. Todos os direitos reservados.
 *
 ***********************************************************************/

#include <stdlib.h>
#include <string.h>
#include <zephyr/shell/shell.h>

#include "actuators/zbook_led.h"

static int parse_led(const struct shell *sh, const char *arg, enum zbook_led *led)
{
	if (strcmp(arg, "all") == 0) {
		*led = ZBOOK_LED_ALL;
		return 0;
	}

	char *end;
	long n = strtol(arg, &end, 10);

	if (*end != '\0' || n < 1 || n > 4) {
		shell_error(sh, "led invalido: %s (use 1-4 ou all)", arg);
		return -EINVAL;
	}

	*led = (enum zbook_led)(n - 1);
	return 0;
}

static int cmd_led_on(const struct shell *sh, size_t argc, char **argv)
{
	enum zbook_led led;
	int ret;

	ret = parse_led(sh, argv[1], &led);
	if (ret < 0) {
		return ret;
	}

	ret = zbook_led_on(led);
	if (ret < 0) {
		shell_error(sh, "zbook_led_on falhou (%d)", ret);
		return ret;
	}

	return 0;
}

static int cmd_led_off(const struct shell *sh, size_t argc, char **argv)
{
	enum zbook_led led;
	int ret;

	ret = parse_led(sh, argv[1], &led);
	if (ret < 0) {
		return ret;
	}

	ret = zbook_led_off(led);
	if (ret < 0) {
		shell_error(sh, "zbook_led_off falhou (%d)", ret);
		return ret;
	}

	return 0;
}

static int cmd_led_blink(const struct shell *sh, size_t argc, char **argv)
{
	enum zbook_led led;
	enum zbook_led_state state;
	int ret;

	ret = parse_led(sh, argv[1], &led);
	if (ret < 0) {
		return ret;
	}

	if (strcmp(argv[2], "default") == 0) {
		state = ZBOOK_LED_BLINK_DEFAULT;
	} else if (strcmp(argv[2], "slow") == 0) {
		state = ZBOOK_LED_BLINK_SLOW;
	} else if (strcmp(argv[2], "fast") == 0) {
		state = ZBOOK_LED_BLINK_FAST;
	} else {
		shell_error(sh, "state invalido: %s (use default, slow ou fast)", argv[2]);
		return -EINVAL;
	}

	ret = zbook_led_blink(led, state);
	if (ret < 0) {
		shell_error(sh, "zbook_led_blink falhou (%d)", ret);
		return ret;
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_led,
			       SHELL_CMD_ARG(on, NULL, "led on <1-4|all>", cmd_led_on, 2, 0),
			       SHELL_CMD_ARG(off, NULL, "led off <1-4|all>", cmd_led_off, 2, 0),
			       SHELL_CMD_ARG(blink, NULL, "led blink <1-4|all> <default|slow|fast>",
					     cmd_led_blink, 3, 0),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(led, &sub_led, "Zbook LED actuator commands", NULL);
