/*************************************************************************
 * @file main.c
 *
 * @brief Entry point for the Zbook button I/O sample: initializes the
 *        buttons and logs every press/release/long-press event through a
 *        single callback registered for all of them.
 * @author João Matheus Nascimento Dias (joao.dias@edge.ufal.br)
 * @version 0.2
 * @date 15/09/2026
 *
 * @copyright Centro de Inovação EDGE 2026. Todos os direitos reservados.
 *
 ***********************************************************************/

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <stdio.h>

#include "io/zbook_btn.h"

LOG_MODULE_REGISTER(main);

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

static void on_btn_evt(enum zbook_btn btn, enum zbook_btn_evt evt, void *user_data)
{
	const char *label = user_data;

	LOG_INF("[%s] BTN%d %s", label, btn, evt_name(evt));
}

int main(void)
{
	LOG_INF("Zbook Button I/O Sample\n");

	int ret = zbook_btn_init();

	if (ret < 0) {
		LOG_ERR("zbook_btn_init failed (%d)", ret);
		return ret;
	}

	ret = zbook_btn_reg_cb(ZBOOK_BTN_ALL, ZBOOK_BTN_EVT_ALL, on_btn_evt, "sample");
	if (ret < 0) {
		LOG_ERR("zbook_btn_reg_cb failed (%d)", ret);
		return ret;
	}

	return 0;
}
