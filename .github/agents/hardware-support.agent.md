---
description: Guide maintainers through Meshtastic hardware support context generation, board intake assessment, and optional scaffold generation.
---

# Hardware Support Workflow

Use this workflow when a maintainer wants to add support for a new board variant in the Meshtastic firmware repository.

For a quick file-template reference (variant.h macros, custom board JSON, InkHUD), see the companion
[new-variant prompt](../prompts/new-variant.prompt.md). This workflow adds the intake assessment,
evidence gating, and compile validation around those templates.

## Goals

- Reuse repository-backed hardware patterns before drafting new board files.
- Keep all generated output scoped to the requested architecture and board.
- Stop and surface evidence gaps instead of inventing pin mappings or metadata.

## Required Inputs

Collect or confirm these fields before scaffold generation:

- PlatformIO environment name
- hardware model identifier — must be a registered `meshtastic_HardwareModel` enum value
  (check `src/mesh/generated/meshtastic/mesh.pb.h`; new models need a meshtastic/protobufs PR first)
- display name
- architecture — one of: `esp32`, `esp32-s3`, `esp32-c3`, `esp32-c6`, `esp32s2`, `esp32p4`,
  `nrf52840`, `nrf54l15`, `rp2040`, `rp2350`, `stm32`, `native`.
  Use the hyphenated ESP32 spellings in metadata (`esp32-s3`, not `esp32s3`); variant
  directories drop the hyphen (`variants/esp32s3/`). `bin/platformio-custom.py` normalizes
  the architecture in the build manifest.

Recommended additional inputs:

- hardware model slug — should match the `HardwareModel` enum entry name (UPPER_SNAKE_CASE)
- actively supported flag
- support level
- board level — CI build tier consumed by `bin/generate_ci_matrix.py`:
  - `pr`: built on every PR (and releases)
  - unset: built on release builds only
  - `extra`: built only on full releases
  - `community`: community-maintained DIY boards, excluded from the CI matrix
- source materials such as schematic, pinout, or datasheet links
- board notes covering revision scope and known uncertainty

## Workflow

### 1. Refresh Repository Context

Run from the repository root:

```bash
python3 bin/generate_hardware_support_context.py
```

This writes two artifacts in lockstep:

- `docs/hardware-support-context.md` — the human/agent view. Emitted prettier-stable;
  no `trunk fmt` pass is needed after regeneration.
- `docs/hardware-support-context.json` — the machine view consumed by `bin/board_intake.py`
  and external tooling (e.g. web dashboards). Includes per-environment `settings`
  (`board`, `board_level`, `board_check`, `extends`) that the markdown tables omit.
  Trunk-ignored as a generated artifact.

The [hardware_support_context workflow](../workflows/hardware_support_context.yml)
regenerates both files when variant changes land on develop and opens a refresh PR,
so local regeneration is only needed when working ahead of CI.

Review [docs/hardware-support-context.md](../../docs/hardware-support-context.md) for architecture-specific examples, metadata keys, and inherited-default notes.

### 2. Capture The Intake Request

Create or update a JSON file matching the contract in [specs/129-hardware-support-agent/contracts/board-intake-contract.md](../../specs/129-hardware-support-agent/contracts/board-intake-contract.md).

### 3. Assess Intake Readiness

Run:

```bash
python3 bin/board_intake.py path/to/intake.json
```

What to look for:

- expected artifacts
- required metadata
- matched repository patterns
- evidence gaps
- risk flags
- next actions
- scaffold readiness decision

For CI-style gating, use:

```bash
python3 bin/board_intake.py path/to/intake.json --validate
```

Programmatic consumers (actions, web tooling) can add `--json` to either command to get
the structured assessment instead of markdown; `board_scaffold.py` accepts `--json` too
and reports generated paths or the blocking assessment.

If the assessment is not scaffold-ready, stop and resolve the blocking gaps before continuing.

### 4. Generate Scaffold Output When Ready

Only run this when the intake assessment reports `Scaffold ready: Yes`.

```bash
python3 bin/board_scaffold.py path/to/intake.json --output-dir generated/hardware-support
```

Expected outputs:

- draft `variant.h`
- draft `platformio.ini`
- draft `variant.cpp`:
  - **nrf52840 / nrf54l15**: required — these families need a real pin description table and
    `initVariant()`. The scaffold emits a stub that will not link; copy the table from the
    matched pattern board and verify every entry against the schematic. The generated
    `platformio.ini` includes the `build_src_filter` entry that compiles it.
  - **ESP32 family**: optional — only for custom init hooks or a `variantDefaultConfig()`
    override (weak hook called from NodeDB to change config defaults for the board).

Review all `// TODO: verify — ...` annotations before treating the scaffold as merge-ready.

### 5. Compile-Gate The Target Environment (Required)

The end stage must always validate that the target environment is at least compilable.

Run:

```bash
pio run -e <environment_name>
```

Expected behavior:

- If compile succeeds, include a "compile check passed" note in the review summary.
- If compile fails, treat it as a blocking issue and report the exact failing error.
- Do not mark the workflow complete while compile is failing.

Common first-pass blockers for new scaffolds:

- Missing or placeholder `board = ...` in `platformio.ini` causes `BoardConfig: Board is not defined`.
- Missing `-I variants/<arch>/<dir>` in `build_flags` — `bin/generate_ci_matrix.py` derives the
  CI platform from this flag and hard-fails the whole matrix for every environment if absent.
- nrf52840/nrf54l15: missing `build_src_filter` entry for the variant directory leaves
  `variant.cpp` uncompiled and surfaces as undefined pin-table symbols at link time.
- ESP32 family builds use Arduino 3.x via pioarduino; the first build downloads large
  toolchain packages. esp32s3 environments have failed to link on macOS hosts before —
  if a local esp32s3 link fails for toolchain (not code) reasons, verify via CI or Docker
  instead of debugging locally.

## Guardrails

- Do not change live firmware runtime code under `src/` as part of this workflow.
- Do not modify existing board definitions under `variants/` automatically.
- Do not guess unresolved radio, display, GPS, power, or input pin mappings.
- Do not invent `custom_meshtastic_hw_model` values — verify against the `HardwareModel`
  enum and flag a protobufs PR as a prerequisite when the model is unregistered.
- Treat multi-revision or multi-option board notes as blocking until the revision scope is explicit.
- Check inherited BSP defaults for `nrf52840`, `nrf54l15`, `rp2040`, `rp2350`, `stm32`, and
  `native` targets before declaring a missing define.

## Repository Conventions To Reuse

- Boards with several build flavors define a shared `[<board>_base]` section in their
  `platformio.ini` and multiple `[env:...]` sections extending it (see
  `variants/esp32s3/heltec_v4_r8/platformio.ini`).
- InkHUD e-ink flavors compose bases: `extends = nrf52840_base, inkhud` plus a
  `nicheGraphics.h` in the variant directory.
- `board_check = true` marks an environment for the CI compile-check matrix.
- `nrf54l15` is Zephyr-based and `esp32p4` has a base environment but no shipped boards yet —
  expect weaker pattern matches and extra manual review on both.

## Validation Expectations

- Run `trunk fmt --force` on touched Python workflow files. The generated context
  markdown and JSON are emitted format-stable and need no fmt pass.
- Re-run `python3 bin/generate_hardware_support_context.py` after changes affecting
  context output, and commit the markdown and JSON together.
- Use fixture-driven smoke tests in `bin/fixtures/` for intake and scaffold workflows.
- Run `pio run -e <environment_name>` as a required final compile gate for the generated board environment.
- Report any skipped validation or remaining TODO annotations in the review notes.
