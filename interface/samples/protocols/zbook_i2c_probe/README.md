
# zbook_i2c_probe

Real-hardware bring-up test for `zbook_i2c` (`interface/includes/protocols/zbook_i2c.h`): the
zbook scans the external GPIO header's I2C bus (H1, I2C_SDA/I2C_SCL) every 2 seconds, and an
nRF52 DK answers as a fake I2C target at address `0x50` -- an address neither on-board device
(TMP1075 0x48, OLED 0x3C) uses, so seeing `0x50` in the scan is real proof the external bus
works.

Two separate Zephyr apps, built and flashed independently:

```
zbook_i2c_probe/
├── zbook_master/    -- uses interface/'s zbook_i2c API, board: zbook/rp2350b/m33
└── nrf52_target/    -- pure Zephyr, no interface/ dependency, board: nrf52dk/nrf52832
```

## Wiring

| nRF52 DK      | zbook (H1 GPIO header) |
| ------------- | ----------------------- |
| P0.02 (SDA)   | I2C_SDA (pin 1)          |
| P0.03 (SCL)   | I2C_SCL (pin 14)         |
| GND           | GND (direct wire -- don't rely on a shared USB ground) |

zbook's I2C_SDA/I2C_SCL already have pull-ups (R69/R70) -- don't add more.

## Build & flash

zbook (master) -- run from the `zbook-sdk` repo root, with `ZEPHYR_BASE` pointing at your
Zephyr checkout for this workspace. `zbook-sdk` and the `zbook` board port aren't registered
west modules by default, so both are passed explicitly:

```sh
export ZEPHYR_BASE=<path-to-your-zephyr-checkout>
west build -b zbook/rp2350b/m33 -d build_master -p always \
  samples/protocols/zbook_i2c_probe/zbook_master -- \
  -DBOARD_ROOT=<path-to-your-zbook-board-port-checkout> \
  -DZEPHYR_EXTRA_MODULES=<path-to-this-zbook-sdk-checkout>

west flash -d build_master \
  --openocd <path-to-an-rp2350-capable-openocd>/bin/openocd \
  --openocd-search <path-to-an-rp2350-capable-openocd>/share/openocd/scripts
```

The zephyr-sdk's bundled OpenOCD has no RP2350 support as of this writing -- build one from
the [Raspberry Pi OpenOCD fork](https://github.com/raspberrypi/openocd) if `west flash` can't
find `target/rp2350.cfg`.

nRF52 DK (target) -- build against any Zephyr checkout that has the `hal_nordic` module
fetched (if this workspace's own `zephyr/` doesn't -- zbook-sdk only pulls the RP2350 modules
it needs -- point at another one that does, or run `west blobs fetch hal_nordic` /
add the module to this workspace's manifest):

```sh
export ZEPHYR_BASE=<path-to-a-zephyr-checkout-with-hal_nordic>
west build -b nrf52dk/nrf52832 -d build_target -p always \
  <path-to-this-zbook-sdk-checkout>/samples/protocols/zbook_i2c_probe/nrf52_target

west flash -d build_target --runner jlink
```

## Observe

zbook: `cat /dev/ttyACM0` (USB CDC via the Raspberry Pi Debug Probe; device path may differ).

nRF52 DK: no USB console -- read over RTT instead:

```sh
JLinkExe -Device NRF52832_XXAA -If SWD -Speed 4000 -AutoConnect 1 \
  -CommandFile <(printf 'r\ng\nqc\n')

JLinkRTTLogger -Device NRF52832_XXAA -If SWD -Speed 4000 -RTTChannel 0 /tmp/rtt.log
```

## Expected output

zbook (master), repeating every 2 seconds:
```
  ACK at 0x3c   <-- on-board OLED, if populated
  ACK at 0x50   <-- the nRF52 DK target below
```

nRF52 DK (target), once per successful probe:
```
I2C TARGET: address matched, master reading -- sending 0xa5
```

If `0x50` never shows up, check the wiring table above and the common ground first.
