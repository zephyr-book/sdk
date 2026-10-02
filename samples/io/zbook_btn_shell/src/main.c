/************************************************************************
 * @file main.c
 *
 * @brief Entry point for the Zbook button I/O sample: initializes the
 *        buttons and exposes the `btn` shell commands to register/remove
 *        event callbacks manually.
 * @author João Matheus Nascimento Dias (joao.dias@edge.ufal.br)
 *
 * @copyright Centro de Inovação EDGE 2026. Todos os direitos reservados.
 *
 ***********************************************************************/

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <stdio.h>

#include "io/zbook_btn.h"

LOG_MODULE_REGISTER(main);

int main(void)
{
	LOG_INF("Zbook Button I/O Sample\n");

	int ret = zbook_btn_init();

	if (ret < 0) {
		LOG_ERR("zbook_btn_init failed (%d)", ret);
	}

	return ret;
}
