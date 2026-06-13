---
description: Bring up a new board from a board packet — intake assessment, schematic pin extraction, variant scaffold, pio build, draft PR
argument-hint: [path to board-packet .zip, packet directory, or intake.json]
---

# `/board-bringup` — board packet → draft variant PR

Take a **board packet** and carry it as far toward a working
variant as the evidence allows: validate the intake, read the schematic, scaffold
the variant, fill every pin you can prove, build it, and open a **draft** PR.

`$ARGUMENTS` is the packet location: a `.zip`, an unpacked packet directory, or a
bare `intake.json`. If empty, look for `/tmp/board-packet` (the CI download path).

## House rules (non-negotiable)

- **Never invent a pin.** Every pin you fill must be traceable to the schematic,
  pinout, or vendor documentation in the packet (or URLs listed in
  `source_materials`). No evidence → leave the `// TODO: verify` in place.
- **Cite evidence.** For every pin you resolve, record where it came from
  (file + page/sheet/net label). The evidence table goes in the PR body.
- **Touch only what the bring-up needs**: the new variant directory, and nothing
  else. Do not edit `src/`, generated protobuf headers, or other variants.
- **Stop on conflicts.** If the intake assessment reports the environment name or
  hardware model already exists, stop and report — do not overwrite an existing
  variant unless the operator explicitly says this is a revision.
- **Draft PR only.** Never mark ready-for-review, never merge.

## Procedure

1. **Unpack.** If given a zip, extract to `/tmp/board-packet`. Identify:
   `intake.json` (required), `source-materials/` (schematics/datasheets),
   `hwmodel.patch` (optional), `README.md`. Read the README — it carries
   the packet's gap notes.

2. **Refresh agent context** (cheap, keeps pattern-matching current):

   ```bash
   python3 bin/generate_hardware_support_context.py
   ```

3. **Intake assessment.**

   ```bash
   python3 bin/board_intake.py <intake.json> --json
   ```

   Parse the result. Blocking evidence gaps you can close by reading the bundled
   source materials are yours to close; anything else (missing architecture,
   name conflicts) → stop and report. Note the `matched_patterns` — those are
   your reference variants for conventions.

4. **HardwareModel sanity.** `hwmodel.patch` applies to **meshtastic/protobufs,
   not this repo** — do not `git am` it here. Check whether the slug from
   `intake.json` already exists in the protobufs submodule / generated headers.
   If it is not merged upstream yet, proceed using the intake's value but flag
   prominently in the PR: _"Depends on protobufs PR adding `<SLUG> = <N>`"_.

5. **Extract pins from the source materials.** Read every file under
   `source-materials/` (PDFs page by page) plus any URLs in
   `source_materials`. Build a pin map with evidence for:
   - LoRa radio: chip (SX1262/SX1268/SX1280/LR1110/RF95…), SPI bus pins, CS,
     RESET, DIO/IRQ, BUSY, RF-switch wiring (DIO2-as-switch? dedicated
     CTX/CSD?), TCXO voltage if shown
   - Power: VEXT/peripheral rails + active level, battery ADC pin, divider
     ratio → `ADC_MULTIPLIER`, charge-status pins
   - I2C bus(es): SDA/SCL per bus, addresses of on-board peripherals
   - Display: controller + interface + pins (or none)
   - GPS: module, UART TX/RX (from the MCU's perspective — beware schematic
     net-name direction), EN/RESET/PPS, baud
   - Inputs/outputs: user button(s), LED(s) + active level, buzzer
   - Anything unusual (FEM/PA, sensors, SD, touch) worth a note
     Cross-check against the `matched_patterns` variants for macro conventions
     (`variants/<arch>/<reference>/variant.h`).

6. **Scaffold.**

   ```bash
   python3 bin/board_scaffold.py <intake.json> --output-dir /tmp/board-scaffold --json
   ```

   Move the generated files into the proper tree location reported by the
   assessment (`variants/<arch-dir>/<environment_name>/`), then fill every
   `// TODO: verify` you have evidence for. Keep unresolved TODOs as TODOs.
   Wire `platformio.ini` per the matched pattern (correct `extends`, board,
   `custom_meshtastic_*` metadata from the intake, `lib_deps` for the display/
   GPS drivers you actually found).

7. **Cut an upstream-ready branch.** The PR must eventually flow
   Meshtastic-Solutions/mainline-firmware → meshtastic/firmware, so the board
   branch must carry **only the variant commit** relative to BOTH repos —
   never the agent/tooling commits of the branch this command runs from, and
   never fork-only commits on this repo's `develop`. Root it on the merge-base
   of the fork's develop and upstream's develop (a commit in both histories):

   ```bash
   git remote add upstream https://github.com/meshtastic/firmware.git 2>/dev/null || true
   git fetch origin develop
   git fetch upstream develop
   MB=$(git merge-base origin/develop upstream/develop)
   git checkout -b board/<environment_name> "$MB"
   ```

   The new variant directory is untracked, so it survives the branch switch.
   Verify nothing else came along: `git status` must show only
   `variants/<arch-dir>/<environment_name>/`.

8. **Build on the board branch** (proves the variant builds against `develop`,
   not against agent-branch extras):

   ```bash
   pio run -e <environment_name>
   ```

   Fix compile errors that stem from the variant files (missing defines, wrong
   base, lib_deps). If the build cannot pass because of genuinely missing
   hardware facts, stop, keep the TODOs, and say so in the PR — a draft PR with
   honest TODOs beats a fabricated green build.

9. **Commit, push, draft PR** (into Meshtastic-Solutions/mainline-firmware,
   base `develop` — same-repo PR, staging review before upstream):

   ```bash
   git add variants/<arch-dir>/<environment_name>/
   git commit -m "Add <display_name> variant scaffold (<environment_name>)"
   git push -u origin board/<environment_name>
   gh pr create --draft --base develop --title "Add <display_name> board support" --body-file <generated body>
   ```

   PR body must include:
   - One-paragraph summary (device, vendor, architecture, radio)
   - **Pin evidence table**: `define → value → source (file, page/net)`
   - **Unresolved TODOs** with what evidence would close each
   - Build result (`pio run -e <env>` output tail)
   - Protobufs dependency note if the HW model isn't merged upstream
   - A **"Promote to upstream"** section with the exact command a maintainer
     runs once this passes review and hardware verification:
     ```bash
     gh pr create --repo meshtastic/firmware --base develop \
       --head Meshtastic-Solutions:board/<environment_name> \
       --title "Add <display_name> board support"
     ```
     (Works because mainline-firmware is a fork of meshtastic/firmware and the
     branch is rooted on develop — the upstream diff is identical to this PR.)

10. **Report.** End with: PR URL, pins resolved vs TODO count, build status, and
    anything a human must verify on physical hardware (ADC multiplier and RF
    switch wiring are the classic ones).
