# interface/tests

Unit tests for the ZBook interface module, using Zephyr's `ztest` framework
and discovered by `twister`.

## Layout

```
tests/unit/
├── unit.cmake              # shared CMake boilerplate, see below
└── <category>/<peripheral>/
    ├── CMakeLists.txt
    ├── prj.conf
    ├── testcase.yaml
    └── src/
        └── main.c
```

`<category>` mirrors `interface/Kconfig.<category>` (`protocols`, `actuators`,
`io`, `sensors`, `storage`). `<peripheral>` mirrors one `CONFIG_ZBOOK_*`
symbol (e.g. `spi`, `uart`).

### Adding a new test

`cmake_minimum_required()`/`find_package()`/`project()` must be literal in
the test's own `CMakeLists.txt` -- CMake scans the top-level list file for
those textually before running anything, so it can't see them through an
`include()` + macro indirection. `unit.cmake` only wires up `target_sources`:

```cmake
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(test_zbook_<peripheral>)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../unit.cmake)
zbook_unit_test()
```

This expects `src/main.c` in the same directory. `prj.conf` and
`testcase.yaml` still need to be written per test (Kconfig symbols and
test id/tags differ per peripheral).

## Conventions

- **Platform:** `platform_allow: native_sim` by default -- unit tests exercise
  pure logic / API contracts and must not need real hardware. If a peripheral
  can't be meaningfully tested without real silicon, tag it `hardware` and
  keep it under `tests/unit/<category>/<peripheral>/` anyway with
  `platform_allow: zbook@p2/rp2350b/m33`; don't invent a separate top-level
  tree for that until there's more than one such case.
- **Test id:** `interface.<category>.<peripheral>` in `testcase.yaml`, tags
  `[interface, <category>, <peripheral>]`.

## Running

```sh
west twister -T interface/tests --platform native_sim -v
```
