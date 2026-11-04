/*************************************************************************
 * @file fake_backends.h
 *
 * @brief Fake zbook_io_* backends that record how the pin factory calls them.
 * @author João Matheus Nascimento Dias (joao.dias@edge.ufal.br)
 * @version 0.1
 * @date 20/10/2026
 *
 * @copyright Centro de Inovação EDGE 2026. Todos os direitos reservados.
 *
 ***********************************************************************/
#ifndef FAKE_BACKENDS_H
#define FAKE_BACKENDS_H

#include "io/zbook_pin.h"

#include <stddef.h>
#include <stdint.h>

#define FAKE_LOG_SIZE 16

enum fake_op {
	FAKE_OP_INIT,
	FAKE_OP_DEINIT,
	FAKE_OP_READ,
	FAKE_OP_WRITE,
};

/** One recorded backend call, in the order it happened. */
struct fake_event {
	enum zbook_pin_header_function function;
	enum fake_op op;
};

/** Per-function knobs and counters. */
struct fake_backend {
	int init_ret;   /**< Value returned by init. */
	int rw_ret;     /**< Value returned by read/write. */
	uint8_t rd_val; /**< Byte (GPIO) or low byte (ADC) delivered by read. */

	int init_calls;
	int deinit_calls;
	int read_calls;
	int write_calls;

	size_t last_len; /**< len of the last read/write. */
	uint8_t last_wr; /**< First byte of the last write. */
};

/** Knobs and counters of one function's fake backend. */
struct fake_backend *fake_backend(enum zbook_pin_header_function function);

/** Number of backend calls recorded since the last fake_reset(). */
int fake_event_count(void);

/** The idx-th recorded backend call, or NULL when out of range. */
const struct fake_event *fake_event(int idx);

/** Clears counters, events and return values. */
void fake_reset(void);

#endif /* FAKE_BACKENDS_H */
