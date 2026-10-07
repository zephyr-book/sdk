/************************************************************************
 * @file main.c
 *
 * @brief Periodically reads the ZBook LDR sensor and logs its
 * light level as a percentage.
 * @author João Matheus Nascimento Dias (joao.dias@edge.ufal.br)
 * @date 17/09/2026
 *
 * @copyright Centro de Inovação EDGE 2026. Todos os direitos reservados.
 *
 ***********************************************************************/

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "sensors/zbook_ldr.h"

LOG_MODULE_REGISTER(zbook_ldr_sample);

int main(void)
{
	int ret;
	uint16_t level;

	ret = zbook_ldr_init();
	if (ret) {
		LOG_ERR("zbook_ldr_init failed (%d)", ret);
		return ret;
	}

	while (1) {
		ret = zbook_ldr_read(&level);
		if (ret) {
			LOG_ERR("zbook_ldr_read failed (%d)", ret);
		} else {
			LOG_INF("Light level: %u%%", level);
		}

		k_sleep(K_SECONDS(1));
	}

	return 0;
}
