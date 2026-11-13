# io/zbook_gpio_shell

Real-hardware bring-up sample for the GPIO function of the header pin factory
(`interface/includes/io/zbook_pin.h`): exposes `in`/`out`/`write`/`read`/`watch`/`release`/`status`
as shell commands, addressed by IO name, so a human can drive any of zbook's header IOs manually
from the console.

The pins come from `interface/`'s own `zbook-sdk` snippet (`interface/snippets/zbook-sdk/`). This
sample doesn't declare an overlay of its own — it requests the snippet (see `sample.yaml`'s
`required_snippets`, or `-S zbook-sdk` below). Each command goes through `zbook_pin_set_function()`,
so whatever function the pin held before is released first.

The PWM/ADC/UART backends are still the mocks in `samples/io/mocks/`; only GPIO is real.

## Wiring

GPIO Header (`H1`, 2x7):

| Header pin | Signal | IO name |
| ---------- | ------ | ------- |
| 5          | GPIO01 | `io01`  |
| 10         | GPIO39 | `io39`  |
| 6          | GPIO45 | `io45`  |
| 9          | GPIO46 | `io46`  |
| 2, 4, 11   | GND    | --      |
| 7          | +5V    | --      |
| 8          | +3V3   | --      |

To validate output: wire header pin → current-limiting resistor (e.g. 220Ω) → LED anode; LED
cathode → any GND pin. To validate input/interrupt: wire header pin to a button/switch that pulls
it to GND, and use the `up` pull.

## Build & flash

```sh
west build -b zbook@p2/rp2350b/m33 samples/io/zbook_gpio_shell -S zbook-sdk
west flash
```

Open a serial console (115200 baud), e.g. `screen /dev/ttyACM0 115200`.

## Commands

```
uart:~$ gpio out io01 high
"io01" output, high
uart:~$ gpio write io01 0
"io01" set to 0
uart:~$ gpio in io01 up
"io01" input, pull up
uart:~$ gpio read io01
"io01": 1
uart:~$ gpio watch io01 falling up
watching "io01" for falling edges, pull up
"io01" (GPIO1) edge, level 0
uart:~$ gpio status
io01: gpio
io39: unconfigured
io45: unconfigured
io46: unconfigured
uart:~$ gpio release io01
"io01" released
```

- `gpio out <io> [low|high|od]`: output starting at that state (default `low`). `od` is
  open-drain: writing `1` releases the line instead of driving it.
- `gpio in <io> [none|up|down]`: input with that pull (default `none`).
- `gpio watch <io> <rising|falling|both> [none|up|down]`: input that prints every debounced edge
  (`CONFIG_ZBOOK_INTERFACE_IO_GPIO_DEBOUNCE_MS`). The callback runs in the system workqueue.
- `gpio write` only works on outputs; `gpio read` works in any direction and returns the pin's
  electrical level. On an open-drain output that is the line level: after `write 1` it reads `0`
  if another device holds the line low.
