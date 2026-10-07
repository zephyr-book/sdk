/************************************************************************
 * @file zbook_ldr.h
 *
 * @brief Defines the Interface for the Zbook light sensor input.
 * @author João Matheus Nascimento Dias (joao.dias@edge.ufal.br)
 * @version 0.1
 * @date 17/09/2026
 *
 * @copyright Centro de Inovação EDGE 2026. Todos os direitos reservados.
 *
 ***********************************************************************/

#ifndef ZBOOK_LDR_H
#define ZBOOK_LDR_H

#include <stdint.h>

/**
 * @brief Verify if the Zbook light sensor interface is ready to be used.
 *
 * @retval 0 Sucess: The ldr and adc is ready to be used.
 * @retval -ENODEV Error: The ldr or adc is not ready.
 * @retval -EINVAL Error: The adc is not configured properly.
 */
int zbook_ldr_init(void);

/**
 * @brief Read the light sensor value.
 *
 * @param value[out] Pointer to a variable that will receive the light sensor value.
 *
 * @retval 0 Sucess: The light sensor value was read successfully.
 * @retval -EINVAL Error: The value pointer is NULL.
 * @retval -EIO Error: The light sensor value get negaitve value or bad read ADC.
 */
int zbook_ldr_read(uint16_t *value);

#endif /* ZBOOK_LDR_H */