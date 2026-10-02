/*******************************************************************
 * @file main.c
 *
 * @brief Real-hardware bring-up sample for the zbook I2C interface: scans
 * the external GPIO header's I2C bus (H1, I2C_SDA/I2C_SCL) every 2 seconds
 * and reports every address that ACKs. A one-shot ztest assertion doesn't
 * give enough visibility for live hardware bring-up -- this keeps scanning
 * and printing indefinitely so wiring changes, resets, or a device being
 * plugged/unplugged show up in real time on the console. Pair with the
 * fixture in samples/protocols/zbook_i2c_probe/nrf52_target/ to prove the
 * external bus reaches a real, independent device.
 *
 * @copyright Copyright (c) Centro de Inovacao EDGE - 2026
 *
 *******************************************************************/

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include "protocols/zbook_i2c.h"

#define SCAN_ADDR_MIN 0x08
#define SCAN_ADDR_MAX 0x77
#define SCAN_PERIOD   K_SECONDS(2)

int main(void)
{
	int ret;

	printk("zbook i2c bus scanner -- probing 0x%02x..0x%02x on the external header (H1)\n",
	       SCAN_ADDR_MIN, SCAN_ADDR_MAX);

	ret = zbook_i2c_init();
	if (ret < 0) {
		printk("zbook_i2c_init failed: %d -- is i2c0 ready?\n", ret);
		return 0;
	}

	const struct zbook_i2c_cfg cfg = {.speed = ZBOOK_I2C_SPEED_STANDARD_100K};

	ret = zbook_i2c_configure(&cfg);
	if (ret < 0) {
		printk("zbook_i2c_configure failed: %d\n", ret);
		return 0;
	}

	while (1) {
		int found = 0;

		for (uint16_t addr = SCAN_ADDR_MIN; addr <= SCAN_ADDR_MAX; addr++) {
			uint8_t dummy;

			if (zbook_i2c_read(addr, &dummy, sizeof(dummy)) == 0) {
				printk("  ACK at 0x%02x\n", addr);
				found++;
			}
		}

		if (found == 0) {
			printk("scan complete -- no device acked (0x%02x-0x%02x)\n",
			       SCAN_ADDR_MIN, SCAN_ADDR_MAX);
		} else {
			printk("scan complete -- %d device(s) found\n", found);
		}

		k_sleep(SCAN_PERIOD);
	}

	return 0;
}
