# protocols/uart sample

Opens one `zbook_uart` channel at runtime via `zbook_uart_init()`
(TX=GP1/RX=GP39 -- picked in `src/main.c`, not fixed in devicetree) and
writes to it once a second.

`zbook-sdk.overlay` only enables the PIO blocks (`pio0`/`pio1`); the pins
are chosen entirely by the caller. GP1 and GP39 need different PIO
addressing windows on RP2350B (a PIO block only addresses 32 GPIOs at a
time), so `zbook_uart_init()` transparently puts TX and RX on different PIO
blocks -- any TX/RX pin combination works, not just ones that share a
window.

Probe GP1 (TX) with a logic analyzer/scope to see the bytes going out.

## Build

Requires the `zbook-sdk` overlay (enables the `pio0`/`pio1` devicetree
nodes) and the P2 board revision (only there are GP1/GP39 unclaimed by
other peripherals):

```sh
west build -b zbook@p2/rp2350b/m33 interface/samples/protocols/uart -- \
  -DEXTRA_DTC_OVERLAY_FILE=interface/snippets/zbook-sdk/zbook-sdk.overlay
```

`-S zbook-sdk` (the snippet form) only resolves once this repo is consumed
as an external module by another west manifest -- built standalone like
above, its own `zephyr/module.yml` isn't picked up as a snippet root, so the
overlay has to be pointed to directly.
