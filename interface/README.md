
# interface

Pure-C hardware abstraction layer for ZBook boards. Zephyr module (see
`../zephyr/module.yml`), auto-discovered by `west` — no manual
`add_subdirectory`/`rsource` needed by whoever consumes this repo.

## Scope

`interface/` is a thin passthrough over Zephyr drivers. It knows nothing
about any language binding or any specific SDK that consumes it. That
knowledge lives one repo over, in `sdk-<lang>` (e.g. `sdk-lua`,
`sdk-micropython`) — never here. Concretely:

- Never `#include` anything from outside this module (no `<lang>.h`, no
  wrapper headers, no wrapper-specific Kconfig symbols).
- A Kconfig symbol here must never be named after a language or wrapper
  (e.g. no `ZBOOK_WRAPPER_<LANG>`). A wrapper that needs a peripheral
  `select`s it from *its own* Kconfig — see [Kconfig](#kconfig) below.
- Return convention: `int`, `0` on success, `-errno` on failure — standard
  Zephyr convention. No business logic beyond what the peripheral itself
  requires (config validation, `configure()`/`transceive()` when the
  hardware genuinely has those primitives).

## Categories

One category per menu in `Kconfig.<category>`, mirrored by an
`includes/<category>/` + `src/<category>/` pair:

| Category      | Kconfig file          | Peripherals (planned)                            |
| ------------- | --------------------- | ------------------------------------------------ |
| `protocols` | `Kconfig.protocols` | UART, SPI, I2C, AT-CMD (wifi/bt)                 |
| `actuators` | `Kconfig.actuators` | LED, RGB-LED, Buzzer, PWM, Display               |
| `io`        | `Kconfig.io`        | Button, rotary encoder, IR                       |
| `sensors`   | `Kconfig.sensors`   | ADC, LDR, temperature sensor, mic, potentiometer |
| `storage`   | `Kconfig.storage`   | SD                                               |

A peripheral goes in the category matching what it *is*, not where it's
used. IMU has no category yet (pending sensor/chip decision) — don't guess,
add a new category or file only once one is chosen.

## Adding a peripheral

Order that keeps every step verifiable before the next:

1. `includes/<category>/zbook_<periph>.h` — public API.
2. Devicetree node `zbook_<periph>` (bound under the right controller, e.g.
   `&spi1`, if it's a bus peripheral) — lives in the board/shield overlay,
   not in this repo.

   Exception: a *shared, multi-function* external header pin (the same
   physical pin usable as GPIO/ADC/PWM/UART-PIO, chosen in software, not by
   board wiring) can't follow that rule — see `zbook_pwm`
   (`interface/snippets/zbook-sdk/`, `interface/src/actuators/zbook_pwm.c`)
   for the pattern: the pin's candidate peripheral nodes live in
   `interface/`'s own snippet, each behind a dedicated, non-default pinctrl
   state that nothing applies automatically, claimed per-pin by
   `zbook_<periph>_init()` at runtime — never merged into the controller's
   own default state, and never in the board repo.
3. `src/<category>/zbook_<periph>.c` — implementation, guarded (see below).
4. Two lines in `Kconfig.<category>`.
5. `tests/unit/<category>/<periph>/` (see `tests/README.md`) — at minimum
   replace the `ztest_test_skip()` placeholder if one already exists there.
6. Validate on real hardware before moving to the next peripheral.

**CMake never changes for a new peripheral.** `interface/CMakeLists.txt`
globs `src/**/*.c` once; enabling/disabling a peripheral is entirely a
Kconfig + `#ifdef` concern (see below) — no `target_sources_ifdef` needed.

### Header — `includes/<category>/zbook_<periph>.h`

```c
int zbook_<periph>_init(void);
int zbook_<periph>_read(...);   /* whatever the peripheral genuinely needs;
                                    add configure()/transceive() etc. if the
                                    hardware has that primitive for real */
```

Same filename stem on both sides (`zbook_spi.h` / `zbook_spi.c`) — no bare
headers like `led.h`. If the peripheral has options (mode, frequency, ...),
use a dedicated `enum` + config struct + a
`zbook_<periph>_configure(const struct zbook_<periph>_cfg *cfg)` function.

### Source — `src/<category>/zbook_<periph>.c`

```c
#include "<category>/zbook_<periph>.h"

#ifdef CONFIG_ZBOOK_<PERIPH>

#include <zephyr/drivers/...>

int zbook_<periph>_init(void)
{
	/* DEVICE_DT_GET / gpio_pin_* / adc_read / spi_transceive_dt / etc. */
}

#endif /* CONFIG_ZBOOK_<PERIPH> */
```

The entire body — including the Zephyr driver `#include`s — sits inside
`#ifdef CONFIG_ZBOOK_<PERIPH>`. Disabled peripherals compile as an empty
translation unit; nothing to wire up in CMake.

## Kconfig

```
interface/Kconfig
  rsource "Kconfig.actuators"
  rsource "Kconfig.io"
  rsource "Kconfig.protocols"
  rsource "Kconfig.sensors"
  rsource "Kconfig.storage"
```

Each `Kconfig.<category>` gets one `config ZBOOK_<PERIPH>` per peripheral:

```kconfig
config ZBOOK_<PERIPH>
	bool "ZBook <PERIPH> protocol interface"
	select <ZEPHYR_DRIVER_CLASS>   # e.g. SPI, SERIAL, GPIO
	help
	  Enable zbook_<periph>_init/read/write, a pure-C passthrough over the
	  Zephyr <driver> bound to the zbook_<periph> devicetree node.
```

Default is `n` — nothing here turns itself on. A wrapper repo enables a
peripheral by `select`ing it from its own Kconfig, never the other way
around:

```kconfig
# sdk-<lang>/Kconfig
config <LANG>_ZBOOK_SPI
	bool "ZBook <lang> binding for SPI"
	default y
	select ZBOOK_SPI
```

This keeps `interface/Kconfig` free of any wrapper-shaped master switch —
multiple wrappers (`sdk-lua`, `sdk-micropython`, ...) can each `select` the
same `ZBOOK_<PERIPH>` symbol without conflict or needing to know about each
other.

## Tests

See `tests/README.md` — one `ztest` app per `<category>/<periph>` under
`tests/unit/`, using the `zbook_unit_test()` CMake helper
(`tests/unit/unit.cmake`).

## Developing a language binding (e.g. sdk-lua) against this repo

`interface` is the manifest/`self` repo — a language binding (`sdk-lua`,
`sdk-micropython`, ...) is a separate git repo, plugged in via its own
`west.yml` project entry with `import: true`. It never builds standalone;
it always needs a workspace rooted at `interface`. This works the same way
whether the binding repo lives on your machine, in your fork, or in the
official org — west only cares about the git remote/revision pinned in the
manifest, not where the repo physically lives.

### Fast local iteration

1. Set up the workspace from this repo:

   ```sh
   west init -m <interface-repo-url> zbook-ws
   cd zbook-ws && west update
   ```
2. Uncomment the binding's project entry (`import: true`) in this repo's
   `west.yml`, then `west update` again — this fetches the binding repo
   (e.g. `sdk-lua`) and its own runtime deps (e.g. `lua_zephyr`) into
   `modules/<binding>`.
3. Swap the fetched checkout for your working clone — it's a plain git
   directory, no west magic involved:

   ```sh
   rm -rf modules/<binding>
   ln -s /path/to/your/local/<binding> modules/<binding>
   ```
4. Build and iterate from the workspace root:

   ```sh
   west build -b zbook@p2/rp2350b/m33 sdk
   ```

   Edits to the symlinked binding repo are picked up immediately — no
   `west update` needed while iterating; it doesn't touch a project you've
   replaced with a symlink.

### Publishing

Once the binding change is ready:

1. `git commit` / `git push` in the binding repo (to your fork or the
   official one), open a PR if applicable.
2. Point this repo's `west.yml` `revision:` for that project at the
   merged commit/tag.
3. Remove the symlink, `west update` for a clean fetch, `west build`
   again — this confirms the change builds for anyone who just clones
   `interface` and runs `west update`, without your local dev setup.
