# zbook_microphone

Real-hardware bring-up sample for `zbook_microphone`
(`interface/includes/sensors/zbook_microphone.h`). A single raw ADC sample
doesn't say much about an AC audio signal on its own, so this bursts as many
reads as it can fit into a 200ms window and reports min/max/peak-to-peak
once a second.

## What to look for

Watch the console while quiet, then make noise (talk, clap) close to the
mic:
```
mic: min=2010 max=2050 peak-to-peak=40      <- quiet, near the DC bias
mic: min=1500 max=2600 peak-to-peak=1100    <- clapping
```
If peak-to-peak barely changes regardless of noise, check the mic wiring
(MICROPHONE_ADC net) and that `microphone_adc` in the board's devicetree is
still channel 0 (GPIO40_ADC0).

## Build & flash

```sh
export ZEPHYR_BASE=<path-to-your-zephyr-checkout>
west build -b zbook/rp2350b/m33 -d build_microphone -p always \
  interface/samples/sensors/zbook_microphone -- \
  -DBOARD_ROOT=<path-to-your-zbook-board-port-checkout> \
  -DZEPHYR_EXTRA_MODULES=<path-to-this-zbook-sdk-checkout>

west flash -d build_microphone \
  --openocd <path-to-an-rp2350-capable-openocd>/bin/openocd \
  --openocd-search <path-to-an-rp2350-capable-openocd>/share/openocd/scripts
```

## Observe

`cat /dev/ttyACM0` (USB CDC via the Raspberry Pi Debug Probe; device path
may differ).
