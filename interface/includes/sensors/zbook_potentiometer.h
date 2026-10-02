/************************************************************************
 * @file zbook_potentiometer.h
 *
 * @brief Defines the Interface for the Zbook potentiometer sensor input.
 * @author João Matheus Nascimento Dias (joao.dias@edge.ufal.br)
 * @version 0.2
 * @date 18/09/2026
 *
 * @copyright Centro de Inovação EDGE 2026. Todos os direitos reservados.
 *
 ***********************************************************************/

#ifndef ZBOOK_POTENTIOMETER_H
#define ZBOOK_POTENTIOMETER_H

#include <stdint.h>

/**
 * @brief Verify if the Zbook potentiometer sensor interface is ready to be used.
 *
 * @retval 0 Sucess: The potentiometer and adc is ready to be used.
 * @retval -ENODEV Error: The potentiometer or adc is not ready.
 * @retval -EINVAL Error: The adc is not configured properly.
 */
int zbook_potentiometer_init(void);

/**
 * @brief Read the potentiometer sensor value.
 *
 * @param value[out] Pointer to a variable that will receive the potentiometer sensor value.
 *
 * @retval 0 Sucess: The potentiometer sensor value was read successfully.
 * @retval -EINVAL Error: The value pointer is NULL.
 * @retval -EIO Error: The potentiometer sensor value get negaitve value.
 * @retval -ERROR Error: The adc get error on read.
 */
int zbook_potentiometer_read(uint16_t *value);

#endif /* ZBOOK_POTENTIOMETER_H */
