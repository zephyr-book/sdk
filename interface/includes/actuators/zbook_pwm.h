/*******************************************************************
 * @file zbook_pwm.h
 *
 * @brief Defines the interface for zbook generic PWM output channels.
 * @author José Félix de Oliveira Neto (josefelix.neto@edge.ufal.br)
 * @version 0.1
 * @date 16/09/26
 *
 * @copyright Copyright (c) 2026
 *
 *******************************************************************/

#ifndef ZBOOK_PWM_H
#define ZBOOK_PWM_H

#include <stdint.h>

/**
 * @brief One of zbook's PWM-capable external header IOs.
 *
 */
enum zbook_pwm_io {
	ZBOOK_PWM_IO01,
	ZBOOK_PWM_IO39,
	ZBOOK_PWM_IO45,
	ZBOOK_PWM_IO46,
};

/** @brief Output polarity for a zbook PWM channel. */
enum zbook_pwm_polarity {
	ZBOOK_PWM_POLARITY_NORMAL = 0, /**< Duty cycle is time spent high. */
	ZBOOK_PWM_POLARITY_INVERTED,   /**< Duty cycle is time spent low. */
};

/** @brief Runtime-configurable zbook PWM channel parameters. */
struct zbook_pwm_cfg {
	uint32_t frequency_hz;            /**< PWM frequency in Hz. Must be nonzero. */
	uint8_t duty_cycle_percent;       /**< Duty cycle, 0 to 100. */
	enum zbook_pwm_polarity polarity; /**< Output polarity. */
};

/**
 * @brief Claim @p io and get it ready to use as a PWM output.
 *
 * Leaves the channel silent (0% duty) on success.
 *
 * @param io One of zbook's PWM-capable header IOs.
 * @retval 0 Success.
 * @retval -ENODEV @p io isn't wired up as PWM in this build, or its
 *                 underlying device isn't ready.
 */
int zbook_pwm_init(enum zbook_pwm_io io);

/**
 * @brief Start @p io at its currently configured frequency/duty
 * cycle/polarity.
 *
 * @param io One of zbook's PWM-capable header IOs.
 * @retval 0 Success.
 * @retval -ENODEV @p io isn't wired up, or zbook_pwm_init() hasn't been
 *                 called successfully for it yet.
 */
int zbook_pwm_start(enum zbook_pwm_io io);

/**
 * @brief Stop @p io.
 *
 * @param io One of zbook's PWM-capable header IOs.
 * @retval 0 Success.
 * @retval -ENODEV @p io isn't wired up, or zbook_pwm_init() hasn't been
 *                 called successfully for it yet.
 */
int zbook_pwm_stop(enum zbook_pwm_io io);

/**
 * @brief Apply @p cfg to @p io.
 *
 * @param io  One of zbook's PWM-capable header IOs.
 * @param cfg Desired configuration. All fields are required.
 * @retval 0 Success.
 * @retval -EINVAL @p cfg is NULL, @p cfg->frequency_hz is 0, or
 *                 @p cfg->duty_cycle_percent is greater than 100.
 * @retval -ENODEV @p io isn't wired up, or zbook_pwm_init() hasn't been
 *                 called successfully for it yet.
 */
int zbook_pwm_set_cfg(enum zbook_pwm_io io, const struct zbook_pwm_cfg *cfg);

/**
 * @brief Read back the configuration last applied to @p io.
 *
 * @param io  One of zbook's PWM-capable header IOs.
 * @param cfg Destination for the current configuration.
 * @retval 0 Success.
 * @retval -EINVAL @p cfg is NULL.
 * @retval -ENODEV @p io isn't wired up, or zbook_pwm_init() hasn't been
 *                 called successfully for it yet.
 */
int zbook_pwm_get_cfg(enum zbook_pwm_io io, struct zbook_pwm_cfg *cfg);

#endif /* ZBOOK_PWM_H */
