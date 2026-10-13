/*******************************************************************
 * @file zbook_microphone.h
 *
 * @brief Defines the Interface for the Zbook microphone sensor input.
 * @author Matheus Macário dos Santos (matheus.macario@edge.ufal.br)
 * @version 0.1
 * @date 23/09/2026
 *
 * @copyright Copyright (c) Centro de Inovacao EDGE - 2026
 *
 *******************************************************************/

#ifndef ZBOOK_MICROPHONE_H
#define ZBOOK_MICROPHONE_H

#include <stdint.h>

/**
 * @brief Verify if the Zbook microphone sensor interface is ready to be used.
 *
 * @retval 0 Sucess: The microphone and adc is ready to be used.
 * @retval -ENODEV Error: The microphone or adc is not ready.
 * @retval -EINVAL Error: The adc is not configured properly.
 */
int zbook_microphone_init(void);

/**
 * @brief Read the raw microphone ADC sample.
 *
 * @param value[out] Pointer to a variable that will receive the raw sample.
 *
 * @retval 0 Sucess: The microphone sample was read successfully.
 * @retval -EINVAL Error: The value pointer is NULL.
 * @retval -ERROR Error: The adc get error on read.
 */
int zbook_microphone_read(uint16_t *value);

#endif /* ZBOOK_MICROPHONE_H */
