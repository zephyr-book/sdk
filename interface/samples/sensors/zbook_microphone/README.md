# zbook_microphone

Sample for the `zbook_microphone` interface
(`interface/includes/sensors/zbook_microphone.h`). A single raw ADC sample
doesn't say much about an AC audio signal on its own, so this bursts as many
reads as it can fit into a sampling window and reports min/max/peak-to-peak,
then sleeps before the next window.

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

## Configuration

| Option                          | Default | Description                              |
|---------------------------------|---------|------------------------------------------|
| `CONFIG_APP_SAMPLE_WINDOW_MS`   | 200     | Duration of each burst of ADC reads (ms) |
| `CONFIG_APP_REPORT_INTERVAL_MS` | 1000    | Sleep after each window (ms)             |

Override them in `prj.conf`, via `west build -t menuconfig`, or on the
command line (`west build ... -- -DCONFIG_APP_SAMPLE_WINDOW_MS=500`).

## Build & flash

From the workspace set up with this repo's `west.yml` (the board and this
module are discovered automatically):
```sh
west build -b zbook@p2/rp2350b/m33 -d build_microphone -p always \
  interface/samples/sensors/zbook_microphone

west flash -d build_microphone \
  --openocd <path-to-an-rp2350-capable-openocd>/bin/openocd \
  --openocd-search <path-to-an-rp2350-capable-openocd>/share/openocd/scripts
```

## Observe

`cat /dev/ttyACM0` (USB CDC via the Raspberry Pi Debug Probe; device path
may differ).
