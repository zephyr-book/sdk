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
 * @retval 0 Success: The ldr and adc are ready to be used.
 * @retval -ENODEV Error: The adc device is not ready.
 * @retval -EIO Error: The adc calibration read failed.
 * @retval <0 Error: Negative errno propagated from adc_channel_setup()
 *         (e.g. -EINVAL, -ENOTSUP) when the channel config is invalid.
 */
int zbook_ldr_init(void);

/**
 * @brief Read the light sensor value.
 *
 * @param[out] value Pointer that receives the light level as a percentage (0-100).
 *
 * @retval 0 Success: The light sensor value was read successfully.
 * @retval -EINVAL Error: The value pointer is NULL.
 * @retval -EIO Error: The adc returned a zero raw sample; value is set to 0.
 * @retval <0 Error: Negative errno propagated from adc_read().
 */
int zbook_ldr_read(uint16_t *value);

#endif /* ZBOOK_LDR_H */