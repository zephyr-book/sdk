/*******************************************************************
 * @file zbook_i2c.c
 *
 * @brief Implements the interface for the zbook I2C bus.
 * @author Matheus Macário dos Santos (matheus.macario@edge.ufal.br)
 * @version 0.1
 * @date 18/09/2026
 * 
 * @copyright Copyright (c) Centro de Inovacao EDGE - 2026
 *
 *******************************************************************/

#include "protocols/zbook_i2c.h"

#ifdef CONFIG_ZBOOK_INTERFACE_PROTOCOLS_I2C

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(zbook_i2c, CONFIG_ZBOOK_I2C_LOG_LEVEL);

static const struct device *const zbook_i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));

static bool i2c_ready = false;

int zbook_i2c_init(void)
{
	if (!device_is_ready(zbook_i2c_dev)) {
		LOG_ERR("zbook_i2c device not ready");
		return -ENODEV;
	}

	i2c_ready = true;
	LOG_DBG("zbook_i2c initialized");

	return 0;
}

int zbook_i2c_configure(const struct zbook_i2c_cfg *cfg)
{
	if (!i2c_ready) {
		LOG_ERR("zbook_i2c not initialized");
		return -ENODEV;
	}

	if (cfg == NULL) {
		LOG_ERR("invalid zbook_i2c config");
		return -EINVAL;
	}

	uint32_t speed;

	switch (cfg->speed) {
	case ZBOOK_I2C_SPEED_STANDARD_100K:
		speed = I2C_SPEED_STANDARD;
		break;
	case ZBOOK_I2C_SPEED_FAST_400K:
		speed = I2C_SPEED_FAST;
		break;
	case ZBOOK_I2C_SPEED_FAST_PLUS_1M:
		speed = I2C_SPEED_FAST_PLUS;
		break;
	default:
		LOG_ERR("invalid zbook_i2c speed: %d", cfg->speed);
		return -EINVAL;
	}

	int ret = i2c_configure(zbook_i2c_dev, I2C_MODE_CONTROLLER | I2C_SPEED_SET(speed));

	if (ret < 0) {
		LOG_ERR("zbook_i2c configure failed: %d", ret);
	} else {
		LOG_DBG("zbook_i2c configured: speed=%d", cfg->speed);
	}

	return ret;
}

int zbook_i2c_write(uint16_t addr, const uint8_t *buf, size_t len)
{
	if (!i2c_ready) {
		LOG_ERR("zbook_i2c not initialized");
		return -ENODEV;
	}

	if (buf == NULL || len == 0) {
		LOG_ERR("invalid zbook_i2c write args");
		return -EINVAL;
	}

	int ret = i2c_write(zbook_i2c_dev, buf, len, addr);

	if (ret < 0) {
		LOG_ERR("zbook_i2c write to 0x%02x failed: %d", addr, ret);
	}

	return ret;
}

int zbook_i2c_read(uint16_t addr, uint8_t *buf, size_t len)
{
	if (!i2c_ready) {
		LOG_ERR("zbook_i2c not initialized");
		return -ENODEV;
	}

	if (buf == NULL || len == 0) {
		LOG_ERR("invalid zbook_i2c read args");
		return -EINVAL;
	}

	int ret = i2c_read(zbook_i2c_dev, buf, len, addr);

	if (ret < 0) {
		LOG_ERR("zbook_i2c read from 0x%02x failed: %d", addr, ret);
	}

	return ret;
}

int zbook_i2c_write_read(uint16_t addr, const uint8_t *write_buf, size_t write_len,
			  uint8_t *read_buf, size_t read_len)
{
	if (!i2c_ready) {
		LOG_ERR("zbook_i2c not initialized");
		return -ENODEV;
	}

	if (write_buf == NULL || write_len == 0 || read_buf == NULL || read_len == 0) {
		LOG_ERR("invalid zbook_i2c write_read args");
		return -EINVAL;
	}

	int ret = i2c_write_read(zbook_i2c_dev, addr, write_buf, write_len, read_buf, read_len);

	if (ret < 0) {
		LOG_ERR("zbook_i2c write_read on 0x%02x failed: %d", addr, ret);
	}

	return ret;
}

#endif /* CONFIG_ZBOOK_INTERFACE_PROTOCOLS_I2C */
