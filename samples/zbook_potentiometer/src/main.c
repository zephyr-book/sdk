/************************************************************************
 * @file main.c
 *
 * @brief Sample: periodically reads the ZBook potentiometer sensor and logs
 * its level as a percentage.
 * @author João Matheus Nascimento Dias (joao.dias@edge.ufal.br)
 * @version 0.1
 * @date 18/09/2026
 *
 * @copyright Centro de Inovação EDGE 2026. Todos os direitos reservados.
 *
 ***********************************************************************/

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "sensors/zbook_potentiometer.h"

LOG_MODULE_REGISTER(zbook_potentiometer_sample);

int main(void)
{
	int ret;
	uint16_t level;

	ret = zbook_potentiometer_init();
	if (ret) {
		LOG_ERR("zbook_potentiometer_init failed (%d)", ret);
		return ret;
	}

	while (1) {
		ret = zbook_potentiometer_read(&level);
		if (ret) {
			LOG_ERR("zbook_potentiometer_read failed (%d)", ret);
		} else {
			LOG_INF("Potentiometer level: %u%%", level);
		}

		k_sleep(K_SECONDS(1));
	}

	return 0;
}
