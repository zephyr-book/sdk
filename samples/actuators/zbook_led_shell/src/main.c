/************************************************************************
 * @file main.c
 *
 * @brief Entry point for the Zbook LED actuator sample: initializes the
 *        LEDs and exposes the `led` shell commands to drive them manually.
 * @author João Matheus Nascimento Dias (joao.dias@edge.ufal.br)
 *
 * @copyright Centro de Inovação EDGE 2026. Todos os direitos reservados.
 *
 ***********************************************************************/

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <stdio.h>

#include "actuators/zbook_led.h"

LOG_MODULE_REGISTER(main);

int main(void)
{
	LOG_INF("Zbook LED Actuator Sample\n");

	int ret = zbook_led_init();

	if (ret < 0) {
		LOG_ERR("zbook_led_init failed (%d)", ret);
	}

	return ret;
}
