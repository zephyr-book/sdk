# Potentiometer sample

Reads the ZBook potentiometer sensor via `zbook_potentiometer_init`/`zbook_potentiometer_read`
and logs the level as a percentage once a second.

## Building and running

```sh
west build -b zbook@p2/rp2350b/m33 interface/samples/sensors/zbook_potentiometer
west flash
```
