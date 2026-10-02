/************************************************************************
 * @file zbook_btn.c
 *
 * @brief Implements the interface for the Zbook button input.
 * @author João Matheus Nascimento Dias (joao.dias@edge.ufal.br)
 * @version 0.2
 * @date 15/09/2026
 *
 * @copyright Centro de Inovação EDGE 2026. Todos os direitos reservados.
 *
 ***********************************************************************/

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/input/input.h>
#include <zephyr/logging/log.h>

#include "io/zbook_btn.h"

LOG_MODULE_REGISTER(zbook_btn, CONFIG_BTN_LOG_LEVEL);

/* The four button aliases are expected to be siblings under a single
 * "gpio-keys" compatible node, each with a zephyr,code identifying it. The
 * whole group is exposed by Zephyr's input subsystem as one input device,
 * which is what we register a single callback against below.
 */
#define BTN0_NODE DT_ALIAS(button0)
#define BTN1_NODE DT_ALIAS(button1)
#define BTN2_NODE DT_ALIAS(button2)
#define BTN3_NODE DT_ALIAS(button3)

#define BUTTONS_NODE DT_PARENT(BTN0_NODE)

#define IS_BTN_VALID(_btn)                                                                         \
	do {                                                                                       \
		if ((_btn) < 0 || (_btn) > ZBOOK_BTN_ALL) {                                        \
			LOG_ERR("Invalid button: %d", (_btn));                                     \
			return -EINVAL;                                                            \
		}                                                                                  \
	} while (0)

#define IS_BTN_EVT_VALID(_evt)                                                                     \
	do {                                                                                       \
		if (((_evt) & ~ZBOOK_BTN_EVT_ALL) != 0 || (_evt) == ZBOOK_BTN_EVT_NONE) {          \
			LOG_ERR("Invalid button event mask: %d", (_evt));                          \
			return -EINVAL;                                                            \
		}                                                                                  \
	} while (0)

static const struct device *buttons_dev = DEVICE_DT_GET(BUTTONS_NODE);

/**
 * @brief Per-button runtime context. Groups the input key code, the
 * long-press timer and the registered user callback in a single place so
 * both the input callback and the timer handler can reach everything via
 * CONTAINER_OF, with no manual index bookkeeping.
 */
struct zbook_btn_ctx {
	enum zbook_btn btn;                /**< The button this context is for */
	uint16_t code;                     /**< The zephyr,code identifying this button */
	enum zbook_btn_evt evt_mask;       /**< The event mask this button is registered for */
	zbook_btn_cb_t cb;                 /**< The callback function for this button */
	void *user_data;                   /**< User-defined data for this button */
	struct k_work_delayable long_work; /**< Deferred work firing the long-press event */
} self[ZBOOK_BTN_ALL] = {
	[ZBOOK_BTN_0] = {.btn = ZBOOK_BTN_0,
			 .code = DT_PROP(BTN0_NODE, zephyr_code),
			 .evt_mask = ZBOOK_BTN_EVT_NONE,
			 .cb = NULL,
			 .user_data = NULL},
	[ZBOOK_BTN_1] = {.btn = ZBOOK_BTN_1,
			 .code = DT_PROP(BTN1_NODE, zephyr_code),
			 .evt_mask = ZBOOK_BTN_EVT_NONE,
			 .cb = NULL,
			 .user_data = NULL},
	[ZBOOK_BTN_2] = {.btn = ZBOOK_BTN_2,
			 .code = DT_PROP(BTN2_NODE, zephyr_code),
			 .evt_mask = ZBOOK_BTN_EVT_NONE,
			 .cb = NULL,
			 .user_data = NULL},
	[ZBOOK_BTN_3] = {.btn = ZBOOK_BTN_3,
			 .code = DT_PROP(BTN3_NODE, zephyr_code),
			 .evt_mask = ZBOOK_BTN_EVT_NONE,
			 .cb = NULL,
			 .user_data = NULL},
};

static struct zbook_btn_ctx *ctx_from_code(uint16_t code)
{
	for (size_t i = 0; i < ZBOOK_BTN_ALL; i++) {
		if (self[i].code == code) {
			return &self[i];
		}
	}

	return NULL;
}

/**
 * @brief Fires once the button has been held for CONFIG_BTN_LONG_PRESS_MS
 * without being released. Runs in the system workqueue context.
 */
static void zbook_btn_long_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct zbook_btn_ctx *ctx = CONTAINER_OF(dwork, struct zbook_btn_ctx, long_work);

	if (ctx->cb != NULL) {
		ctx->cb(ctx->btn, ZBOOK_BTN_EVT_LONG_PRESSED, ctx->user_data);
	}
}

/**
 * @brief Input subsystem callback for the buttons device. Runs in whatever
 * context CONFIG_INPUT_MODE_* dispatches events in (a dedicated thread by
 * default), never in ISR context, so calling the user callback directly here
 * is safe.
 */
static void zbook_btn_input_cb(struct input_event *evt, void *user_data)
{
	ARG_UNUSED(user_data);

	if (evt->type != INPUT_EV_KEY) {
		return;
	}

	struct zbook_btn_ctx *ctx = ctx_from_code(evt->code);

	if (ctx == NULL) {
		return;
	}

	if (evt->value) {
		if (ctx->evt_mask & ZBOOK_BTN_EVT_LONG_PRESSED) {
			k_work_schedule(&ctx->long_work, K_MSEC(CONFIG_BTN_LONG_PRESS_MS));
		}

		if ((ctx->evt_mask & ZBOOK_BTN_EVT_PRESSED) && ctx->cb != NULL) {
			ctx->cb(ctx->btn, ZBOOK_BTN_EVT_PRESSED, ctx->user_data);
		}
	} else {
		k_work_cancel_delayable(&ctx->long_work);

		if ((ctx->evt_mask & ZBOOK_BTN_EVT_RELEASED) && ctx->cb != NULL) {
			ctx->cb(ctx->btn, ZBOOK_BTN_EVT_RELEASED, ctx->user_data);
		}
	}
}

INPUT_CALLBACK_DEFINE(DEVICE_DT_GET(BUTTONS_NODE), zbook_btn_input_cb, NULL);

int zbook_btn_init(void)
{
	if (!device_is_ready(buttons_dev)) {
		LOG_ERR("Buttons device not ready");
		return -ENODEV;
	}

	for (size_t i = 0; i < ZBOOK_BTN_ALL; i++) {
		k_work_init_delayable(&self[i].long_work, zbook_btn_long_work_handler);
	}

	return 0;
}

static void zbook_btn_reg_cb_single(struct zbook_btn_ctx *ctx, enum zbook_btn_evt evt,
				    zbook_btn_cb_t cb, void *user_data)
{
	ctx->evt_mask = evt;
	ctx->cb = cb;
	ctx->user_data = user_data;
}

int zbook_btn_reg_cb(enum zbook_btn btn, enum zbook_btn_evt evt, zbook_btn_cb_t cb, void *user_data)
{
	IS_BTN_VALID(btn);
	IS_BTN_EVT_VALID(evt);

	if (cb == NULL) {
		LOG_ERR("Callback must not be NULL");

		return -EINVAL;
	}

	if (btn == ZBOOK_BTN_ALL) {
		for (size_t i = 0; i < ZBOOK_BTN_ALL; i++) {
			zbook_btn_reg_cb_single(&self[i], evt, cb, user_data);
		}

		return 0;
	}

	zbook_btn_reg_cb_single(&self[btn], evt, cb, user_data);

	return 0;
}

static void zbook_btn_rm_cb_single(struct zbook_btn_ctx *ctx)
{
	k_work_cancel_delayable(&ctx->long_work);

	ctx->evt_mask = ZBOOK_BTN_EVT_NONE;
	ctx->cb = NULL;
	ctx->user_data = NULL;
}

int zbook_btn_rm_cb(enum zbook_btn btn)
{
	IS_BTN_VALID(btn);

	if (btn == ZBOOK_BTN_ALL) {
		for (size_t i = 0; i < ZBOOK_BTN_ALL; i++) {
			zbook_btn_rm_cb_single(&self[i]);
		}

		return 0;
	}

	zbook_btn_rm_cb_single(&self[btn]);

	return 0;
}
