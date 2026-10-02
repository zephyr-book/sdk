/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) Centro de Inovacao EDGE - 2026
 */

#include <errno.h>
#include <zephyr/ztest.h>
#include <protocols/zbook_i2c.h>

#include "zbook_i2c_loopback_emul.h"

/* Matches boards/native_sim.overlay's zbook_i2c_target@50 -- a real,
 * ACKing device on the emulated bus, backed by zbook_i2c_loopback_emul.c.
 */
#define ZBOOK_I2C_TEST_ADDR 0x50

/* Reserved by the I2C spec (0x78-0x7F) -- no compliant device may claim
 * this address, so it doubles as a guaranteed-empty target for exercising
 * the "nothing answered" error path.
 */
#define ZBOOK_I2C_RESERVED_ADDR 0x7F

ZTEST_SUITE(zbook_i2c, NULL, NULL, NULL, NULL, NULL);

ZTEST(zbook_i2c, test_00_not_ready_before_init)
{
	uint8_t byte = 0;

	zassert_equal(-ENODEV, zbook_i2c_read(ZBOOK_I2C_TEST_ADDR, &byte, sizeof(byte)));
}

ZTEST(zbook_i2c, test_00a_configure_before_init)
{
	struct zbook_i2c_cfg cfg = {.speed = ZBOOK_I2C_SPEED_STANDARD_100K};

	zassert_equal(-ENODEV, zbook_i2c_configure(&cfg));
}

ZTEST(zbook_i2c, test_01_init_ok)
{
	zassert_ok(zbook_i2c_init());
}

ZTEST(zbook_i2c, test_configure_invalid_args)
{
	struct zbook_i2c_cfg cfg = {.speed = (enum zbook_i2c_speed)99};

	zassert_equal(-EINVAL, zbook_i2c_configure(NULL));
	zassert_equal(-EINVAL, zbook_i2c_configure(&cfg));
}

ZTEST(zbook_i2c, test_configure_accepts_every_supported_speed)
{
	const enum zbook_i2c_speed speeds[] = {
		ZBOOK_I2C_SPEED_STANDARD_100K,
		ZBOOK_I2C_SPEED_FAST_400K,
		ZBOOK_I2C_SPEED_FAST_PLUS_1M,
	};

	for (size_t i = 0; i < ARRAY_SIZE(speeds); i++) {
		const struct zbook_i2c_cfg cfg = {.speed = speeds[i]};

		zassert_ok(zbook_i2c_configure(&cfg), "configure(speed=%d) failed", speeds[i]);
	}
}

ZTEST(zbook_i2c, test_write_invalid_args)
{
	uint8_t byte = 0;

	zassert_equal(-EINVAL, zbook_i2c_write(ZBOOK_I2C_TEST_ADDR, NULL, 1));
	zassert_equal(-EINVAL, zbook_i2c_write(ZBOOK_I2C_TEST_ADDR, &byte, 0));
}

ZTEST(zbook_i2c, test_write_captures_bytes)
{
	uint8_t tx[3] = {0x01, 0x02, 0x03};
	uint8_t captured[3] = {0};

	zbook_i2c_test_emul_reset();
	zassert_ok(zbook_i2c_write(ZBOOK_I2C_TEST_ADDR, tx, sizeof(tx)));
	zassert_equal(sizeof(tx), zbook_i2c_test_emul_last_write(captured, sizeof(captured)));
	zassert_mem_equal(tx, captured, sizeof(tx));
}

ZTEST(zbook_i2c, test_read_invalid_args)
{
	uint8_t byte = 0;

	zassert_equal(-EINVAL, zbook_i2c_read(ZBOOK_I2C_TEST_ADDR, NULL, 1));
	zassert_equal(-EINVAL, zbook_i2c_read(ZBOOK_I2C_TEST_ADDR, &byte, 0));
}

ZTEST(zbook_i2c, test_read_returns_canned_pattern)
{
	uint8_t rx[4] = {0};

	zbook_i2c_test_emul_set_read_byte(0x5A);
	zassert_ok(zbook_i2c_read(ZBOOK_I2C_TEST_ADDR, rx, sizeof(rx)));
	for (size_t i = 0; i < sizeof(rx); i++) {
		zassert_equal(0x5A, rx[i]);
	}
}

ZTEST(zbook_i2c, test_write_read_invalid_args)
{
	uint8_t byte = 0;

	zassert_equal(-EINVAL, zbook_i2c_write_read(ZBOOK_I2C_TEST_ADDR, NULL, 1, &byte, 1));
	zassert_equal(-EINVAL, zbook_i2c_write_read(ZBOOK_I2C_TEST_ADDR, &byte, 0, &byte, 1));
	zassert_equal(-EINVAL, zbook_i2c_write_read(ZBOOK_I2C_TEST_ADDR, &byte, 1, NULL, 1));
	zassert_equal(-EINVAL, zbook_i2c_write_read(ZBOOK_I2C_TEST_ADDR, &byte, 1, &byte, 0));
}

ZTEST(zbook_i2c, test_write_read_combines_write_and_read)
{
	uint8_t reg = 0x10;
	uint8_t rx[2] = {0};
	uint8_t captured[1] = {0};

	zbook_i2c_test_emul_reset();
	zbook_i2c_test_emul_set_read_byte(0x7E);
	zassert_ok(zbook_i2c_write_read(ZBOOK_I2C_TEST_ADDR, &reg, sizeof(reg), rx, sizeof(rx)));

	zassert_equal(sizeof(reg), zbook_i2c_test_emul_last_write(captured, sizeof(captured)));
	zassert_equal(reg, captured[0]);
	zassert_equal(0x7E, rx[0]);
	zassert_equal(0x7E, rx[1]);
}

ZTEST(zbook_i2c, test_write_with_nothing_attached_fails)
{
	uint8_t byte = 0;

	zassert_not_equal(zbook_i2c_write(ZBOOK_I2C_RESERVED_ADDR, &byte, sizeof(byte)), 0,
			   "write() must fail when no device acks");
}

ZTEST(zbook_i2c, test_read_with_nothing_attached_fails)
{
	uint8_t byte = 0;

	zassert_not_equal(zbook_i2c_read(ZBOOK_I2C_RESERVED_ADDR, &byte, sizeof(byte)), 0,
			   "read() must fail when no device acks");
}

ZTEST(zbook_i2c, test_write_read_with_nothing_attached_fails)
{
	uint8_t tx = 0;
	uint8_t rx = 0;

	zassert_not_equal(
		zbook_i2c_write_read(ZBOOK_I2C_RESERVED_ADDR, &tx, sizeof(tx), &rx, sizeof(rx)), 0,
		"write_read() must fail when no device acks");
}
