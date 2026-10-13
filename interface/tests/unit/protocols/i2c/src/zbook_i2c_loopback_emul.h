/*******************************************************************
 * @file zbook_i2c_loopback_emul.h
 *
 * @brief I2C loopback emulator test helpers for zbook_i2c's unit tests.
 * @author Matheus Macário dos Santos (matheus.macario@edge.ufal.br)
 * @version 0.1
 * @date 18/09/2026
 *
 * @copyright Copyright (c) Centro de Inovacao EDGE - 2026
 *
 *******************************************************************/

#ifndef ZBOOK_I2C_LOOPBACK_EMUL_H
#define ZBOOK_I2C_LOOPBACK_EMUL_H

#include <stddef.h>
#include <stdint.h>

/** @brief Reset all captured state (last write, read byte) to zero. */
void zbook_i2c_test_emul_reset(void);

/**
 * @brief Copy the bytes captured from the last write half of a transfer
 * (a plain zbook_i2c_write(), or the write half of zbook_i2c_write_read()).
 *
 * @param out     Destination buffer.
 * @param max_len Capacity of @p out.
 * @return Number of bytes copied.
 */
size_t zbook_i2c_test_emul_last_write(uint8_t *out, size_t max_len);

/**
 * @brief Byte value the emulator stuffs into every read half of a transfer
 * (a plain zbook_i2c_read(), or the read half of zbook_i2c_write_read()).
 */
void zbook_i2c_test_emul_set_read_byte(uint8_t value);

#endif /* ZBOOK_I2C_LOOPBACK_EMUL_H */
