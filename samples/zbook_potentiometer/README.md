# Potentiometer sample

Reads the ZBook potentiometer sensor via `zbook_potentiometer_init`/`zbook_potentiometer_read`
and logs the level as a percentage once a second.

## Devicetree

The zbook board devicetree already defines `potentiometer` as channel 0 of
the `adc` controller, matching what
`interface/src/sensors/zbook_potentiometer.c` looks up via
`ADC_CHANNEL_CFG_DT(DT_NODELABEL(potentiometer))`. Adjust the channel and
`zephyr,*` properties there to match how the potentiometer's wiper is
actually wired.

## Building and running

```sh
west build -b zbook@p2/rp2350b/m33 interface/samples/sensors/zbook_potentiometer
west flash
```
