# actuators/zbook_pwm_shell

Real-hardware bring-up sample for `zbook_pwm` (`interface/includes/actuators/zbook_pwm.h`):
exposes `init`/`start`/`stop`/`set`/`get` as shell commands, addressed by IO name, so a human can
drive any of zbook's PWM-capable header IOs manually from the console.

`zbook_pwm` places no restriction on how many channels exist in principle, but which pins are
available and what they're wired to is fixed by `interface/`'s own `zbook-sdk` snippet
(`interface/snippets/zbook-sdk/`), not by this sample or any consumer-authored devicetree. This
sample doesn't declare an overlay of its own — it just requests the snippet (see `sample.yaml`'s
`required_snippets`, or `-S zbook-sdk` on the command line below), which wires up all four of
ZBook's flexible header IOs (`enum zbook_pwm_io` in `zbook_pwm.h`) as PWM channels. Those same
pins are also selectable as GPIO/ADC/UART-PIO by other `zbook_<periph>` interfaces, none of which
exist yet. Adding a fifth channel means extending the snippet's overlay and `zbook_pwm.c` itself
(see the comments there) — still entirely inside `interface/`, never in this sample or the board
module.

## Wiring

GPIO Header (`H1`, 2x7):

| Header pin | Signal | `zbook_pwm_io` |
| ---------- | ------ | -------------- |
| 5          | GPIO01 | `ZBOOK_PWM_IO01` / `"io01"` |
| 10         | GPIO39 | `ZBOOK_PWM_IO39` / `"io39"` |
| 6          | GPIO45 | `ZBOOK_PWM_IO45` / `"io45"` |
| 9          | GPIO46 | `ZBOOK_PWM_IO46` / `"io46"` |
| 2, 4, 11   | GND    | -- |
| 7          | +5V    | -- |
| 8          | +3V3   | -- |

Pick one IO, wire: header pin → current-limiting resistor (e.g. 220Ω) → LED anode; LED cathode →
any GND pin (2, 4, or 11). GPIO39 and GPIO46 share PWM slice 11 (channels B/A) -- if driving both
at once, they can have independent duty cycles but always the same frequency as each other.

## Build & flash

```sh
west build -b zbook@p2/rp2350b/m33 samples/actuators/zbook_pwm_shell -S zbook-sdk
west flash
```

Open a serial console (115200 baud), e.g. `screen /dev/ttyACM0 115200`.

## Commands

```
uart:~$ pwm init io01
"io01" initialized
uart:~$ pwm set io01 200 0 normal
"io01" set to 200 Hz, 0% duty, normal
uart:~$ pwm start io01
"io01" started
uart:~$ pwm set io01 200 50 normal
"io01" set to 200 Hz, 50% duty, normal
uart:~$ pwm get io01
"io01": 200 Hz, 50% duty, normal
uart:~$ pwm stop io01
"io01" stopped
```

`pwm init <io>` must run once per IO before any other command on it (`<io>` is one of `io01`/
`io39`/`io45`/`io46`) -- unlike earlier versions of this sample, nothing is initialized
automatically at boot, since there's no longer a single "default" channel. `pwm set <io>
<freq_hz> <duty 0-100> <normal|inverted>` takes effect immediately whether the IO is currently
started or stopped -- if stopped, the LED stays off until the next `pwm start`, but the
frequency/duty/polarity are already applied for when it does. `pwm start`/`pwm stop` only toggle
the duty cycle between 0% and whatever `pwm set` last configured; the rest of the config is kept
across a stop. `pwm get` reads back the last config applied via `pwm set` (or the default seeded
by `pwm init`) -- not a live hardware read-back, since the PWM driver has none.
