/*
 * I2C loopback emulator backing the "zbook_i2c_target" devicetree node for
 * unit tests: on a write-only transfer it just records the bytes, on a
 * read-only transfer it stuffs the buffer with a fixed byte set via
 * zbook_i2c_test_emul_set_read_byte(), and on a write-then-read transfer
 * (zbook_i2c_write_read()'s repeated-start pattern) it does both -- records
 * the write half and answers the read half with the same canned byte. This
 * is a fixed address on the emulated i2c0 bus, distinct from the reserved
 * address main.c uses for "nothing attached" -- see boards/native_sim.overlay.
 */

#include "zbook_i2c_loopback_emul.h"

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/sys/util.h>

#define ZBOOK_I2C_TEST_NODE DT_NODELABEL(zbook_i2c_target)
#define MAX_CAPTURE_LEN     32

struct zbook_i2c_loopback_data {
	uint8_t last_write[MAX_CAPTURE_LEN];
	size_t last_write_len;
	uint8_t read_byte;
};

static struct zbook_i2c_loopback_data emul_data;

void zbook_i2c_test_emul_reset(void)
{
	memset(&emul_data, 0, sizeof(emul_data));
}

size_t zbook_i2c_test_emul_last_write(uint8_t *out, size_t max_len)
{
	size_t len = MIN(emul_data.last_write_len, max_len);

	memcpy(out, emul_data.last_write, len);

	return len;
}

void zbook_i2c_test_emul_set_read_byte(uint8_t value)
{
	emul_data.read_byte = value;
}

static void capture_write(struct zbook_i2c_loopback_data *data, struct i2c_msg *msg)
{
	size_t len = MIN(msg->len, MAX_CAPTURE_LEN);

	memcpy(data->last_write, msg->buf, len);
	data->last_write_len = len;
}

static void fill_read(struct zbook_i2c_loopback_data *data, struct i2c_msg *msg)
{
	memset(msg->buf, data->read_byte, msg->len);
}

static int zbook_i2c_loopback_transfer(const struct emul *target, struct i2c_msg *msgs,
					int num_msgs, int addr)
{
	struct zbook_i2c_loopback_data *data = target->data;

	ARG_UNUSED(addr);

	if (num_msgs < 1 || num_msgs > 2) {
		return -EIO;
	}

	if (msgs[0].flags & I2C_MSG_READ) {
		fill_read(data, &msgs[0]);
	} else {
		capture_write(data, &msgs[0]);
	}

	if (num_msgs == 2) {
		if (!(msgs[1].flags & I2C_MSG_READ)) {
			return -EIO;
		}
		fill_read(data, &msgs[1]);
	}

	return 0;
}

static int zbook_i2c_loopback_init(const struct emul *target, const struct device *parent)
{
	ARG_UNUSED(target);
	ARG_UNUSED(parent);

	return 0;
}

/*
 * zbook_i2c has no production driver of its own at this node (it's a bare
 * test target, only ever touched via zbook_i2c_write/read/write_read with a
 * runtime address) so there is normally no struct device here. EMUL_DT_DEFINE()'s
 * .dev = DEVICE_DT_GET(node_id) is dereferenced by emul_init_for_bus() to
 * match this node by name, so this no-op device exists purely to satisfy
 * the emulator framework in this test binary.
 */
static int zbook_i2c_loopback_dev_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

DEVICE_DT_DEFINE(ZBOOK_I2C_TEST_NODE, zbook_i2c_loopback_dev_init, NULL, NULL, NULL, POST_KERNEL,
		 CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);

static const struct i2c_emul_api zbook_i2c_loopback_api = {
	.transfer = zbook_i2c_loopback_transfer,
};

EMUL_DT_DEFINE(ZBOOK_I2C_TEST_NODE, zbook_i2c_loopback_init, &emul_data, NULL,
	       &zbook_i2c_loopback_api, NULL);
