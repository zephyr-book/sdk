/************************************************************************
 * @file main.c
 *
 * @brief Entry point for the Zbook IR I/O shell sample: initializes the IR
 *        emitter and receiver and exposes the `ir` shell commands to drive
 *        them manually.
 * @author Matheus Macário dos Santos (matheus.macario@edge.ufal.br)
 * @version 0.1
 * @date 10/11/2026
 *
 * @copyright Centro de Inovação EDGE 2026. Todos os direitos reservados.
 *
 ***********************************************************************/

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "io/zbook_ir.h"

LOG_MODULE_REGISTER(main);

int main(void)
{
	LOG_INF("Zbook IR I/O Shell Sample");

	int ret = zbook_ir_init();

	if (ret < 0) {
		LOG_ERR("zbook_ir_init failed (%d)", ret);
	}

	return ret;
}
