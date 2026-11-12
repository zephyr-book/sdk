/************************************************************************
 * @file main.c
 *
 * @brief Entry point for the Zbook IR I/O sample: periodically pulses the
 *        IR emitter and logs every DETECTED/LOST event of the IR receiver,
 *        as a reflective test of the emitter/receiver pair.
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

BUILD_ASSERT(CONFIG_SAMPLE_IR_PULSE_MS < CONFIG_SAMPLE_IR_PULSE_PERIOD_MS,
	     "The IR pulse must be shorter than the pulse period");

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

static void on_ir_evt(enum zbook_ir_evt evt, void *user_data)
{
	const char *label = user_data;

	LOG_INF("[%s] IR %s", label, evt_name(evt));
}

int main(void)
{
	int ret;

	LOG_INF("Zbook IR I/O Sample");
	LOG_INF("Hold a surface over the IR emitter/receiver pair to see the pulses");

	ret = zbook_ir_init();
	if (ret < 0) {
		LOG_ERR("zbook_ir_init failed (%d)", ret);
		return ret;
	}

	ret = zbook_ir_reg_cb(ZBOOK_IR_EVT_BOTH, on_ir_evt, "sample");
	if (ret < 0) {
		LOG_ERR("zbook_ir_reg_cb failed (%d)", ret);
		return ret;
	}

	while (1) {
		ret = zbook_ir_emitter_pulse(CONFIG_SAMPLE_IR_PULSE_MS);
		if (ret < 0) {
			LOG_ERR("zbook_ir_emitter_pulse failed (%d)", ret);
			return ret;
		}

		k_msleep(CONFIG_SAMPLE_IR_PULSE_PERIOD_MS);
	}

	return 0;
}
