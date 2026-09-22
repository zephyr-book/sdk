# zbook_led_rgb_probe

Real-hardware bring-up sample for `zbook_led_rgb`
(`interface/includes/actuators/zbook_led_rgb.h`): cycles the 4 on-board
addressable LEDs through red/green/blue/white, one at a time, then all
together, then a mask pattern (even vs odd LEDs). Meant to be watched, not
asserted on -- no second board needed, these LEDs are on the zbook itself.

## What to look for

- **One-by-one phase:** each LED should show the announced color in turn,
  in the right order, with the right colors -- a wrong color on one LED
  usually means its `color-mapping` (channel order) is off in the board's
  devicetree.
- **All-together phase:** all 4 LEDs should show the same color at once. If
  only LED0 lights up here (and in the one-by-one phase, LEDs 1-3 never lit
  either), the `led-strip` devicetree node's `chain-length` doesn't match
  how many LEDs are actually wired on the chain.
- **Mask phase:** LED0+LED2 and LED1+LED3 should alternate, each pair off
  while the other is lit.

## Build & flash

```sh
export ZEPHYR_BASE=<path-to-your-zephyr-checkout>
west build -b zbook/rp2350b/m33 -d build_led_rgb_probe -p always \
  interface/samples/actuators/zbook_led_rgb_probe -- \
  -DBOARD_ROOT=<path-to-your-zbook-board-port-checkout> \
  -DZEPHYR_EXTRA_MODULES=<path-to-this-zbook-sdk-checkout>

west flash -d build_led_rgb_probe \
  --openocd <path-to-an-rp2350-capable-openocd>/bin/openocd \
  --openocd-search <path-to-an-rp2350-capable-openocd>/share/openocd/scripts
```

## Observe

`cat /dev/ttyACM0` (USB CDC via the Raspberry Pi Debug Probe; device path
may differ) -- prints which LED/color is active at each step, e.g.:

```
zbook RGB LED bring-up sample
-- one LED at a time --
  LED0 -> red
  LED0 -> green
  LED0 -> blue
  LED0 -> white
  LED1 -> red
  ...
-- all LEDs together --
  ALL -> red
  ...
-- mask: even vs odd LEDs --
  LED0+LED2 -> red, LED1+LED3 -> off
  LED1+LED3 -> blue, LED0+LED2 -> off
```
