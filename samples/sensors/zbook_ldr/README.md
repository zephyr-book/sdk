# LDR sample

Reads the ZBook LDR light sensor via `zbook_ldr_init`/`zbook_ldr_read` and
logs the light level as a percentage once a second.

## Devicetree

`app.overlay` adds `ldr_adc` as channel 0 of the `adc` controller, matching
what `interface/src/sensors/zbook_ldr.c` looks up via
`ADC_CHANNEL_CFG_DT(DT_NODELABEL(ldr_adc))`. Adjust the channel and
`zephyr,*` properties to match how the photoresistor's voltage divider is
actually wired.

## Building and running

```sh
west build -b zbook@p2/rp2350b/m33 interface/samples/sensors/zbook_ldr
west flash
```
