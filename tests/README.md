# Tests

Unit tests for `sdk-zbook-interface`, run with Twister.

## Running

From the repo root:

```sh
west twister -T tests --platform native_sim -v
```

- `-T tests` — search this directory for `testcase.yaml` files.
- `--platform native_sim` — run on the host instead of target hardware.
- `-v` — verbose output.

## With coverage

```sh
west twister -T tests --platform native_sim --coverage --coverage-basedir . -v
```

- `--coverage` — build with gcov and generate an HTML report.
- `--coverage-basedir .` — scope the report to this repo's own source files, excluding Zephyr framework/kernel code pulled into the build.

Report output: `twister-out/coverage/index.html`.
