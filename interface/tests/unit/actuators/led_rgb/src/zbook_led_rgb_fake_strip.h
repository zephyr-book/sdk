/*******************************************************************
 * @file zbook_led_rgb_fake_strip.h
 *
 * @brief Fake led_strip test helpers for zbook_led_rgb's unit tests.
 * @author Matheus Macário dos Santos (matheus.macario@edge.ufal.br)
 * @version 0.1
 * @date 21/09/2026
 *
 * @copyright Copyright (c) Centro de Inovacao EDGE - 2026
 *
 *******************************************************************/

#ifndef ZBOOK_LED_RGB_FAKE_STRIP_H
#define ZBOOK_LED_RGB_FAKE_STRIP_H

#include <stddef.h>
#include <zephyr/drivers/led_strip.h>

/** @brief Reset captured state to zero. */
void zbook_led_rgb_test_fake_reset(void);

/**
 * @brief Copy the pixel buffer captured from the last led_strip_update_rgb()
 * call made against the fake strip.
 *
 * @param out     Destination buffer.
 * @param max_len Capacity of @p out, in pixels.
 * @return Number of pixels copied.
 */
size_t zbook_led_rgb_test_fake_last_update(struct led_rgb *out, size_t max_len);

#endif /* ZBOOK_LED_RGB_FAKE_STRIP_H */
