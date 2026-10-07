# LDR sample

Reads the ZBook LDR light sensor via `zbook_ldr_init`/`zbook_ldr_read` and
logs the light level as a percentage once a second.

## Building and running

```sh
west build -b zbook@p2/rp2350b/m33 interface/samples/sensors/zbook_ldr
west flash
```
