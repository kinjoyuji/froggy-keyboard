# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this repo is

Personal firmware assets for a yushakobo "Froggy" one-handed keyboard (Helix-based kit, built **left-hand**) with a BLE Micro Pro (BMP, nRF52840) controller and yushakobo's coin-cell battery board. Only self-written/tuned files live here.

The firmware base is the fork `kinjoyuji/qmk_firmware` pinned at `FROGGY_FORK_REF` (default `46b3074d6a4b31d05af124ddcfa539b6b8ff84aa`, set in `build.sh`). It is sekigon-gonnoc's `dev/ble_micro_pro` (2023-07; QMK core from ~2021), so current docs.qmk.fm APIs may not apply. **Never edit the fork from here**, and never run `make` or git write commands in a user's own fork checkout.

## Layout and build

| Path | Purpose |
|---|---|
| `keymap/` | Copied as the whole of the fork's `keyboards/ble_micro_pro/keymaps/froggy/` at build time |
| `bmp-config/` | Byte-exact backup of the BMP drive's `KEYMAP.JSN`, `CONFIG.JSN`, `TAPTERM.JSN`, `ENCODER.JSN`, `MACRO.TXT` (`-text` in `.gitattributes`; never reformat or change line endings) |
| `build.sh` | Builds `build/ble_micro_pro_froggy.uf2` |
| `scripts/keymap-table.py` | Regenerates the README "キー配置一覧" block from `bmp-config/KEYMAP.JSN`; run it after every `KEYMAP.JSN` change (`--check` exits 1 if stale). Layer names and key labels live in the script |
| `scripts/bmp-config.sh` | `locate` / `backup` / `restore [--yes] FILE...` between `bmp-config/` and the BMP drive |

```sh
./build.sh                                                # clone from GitHub (partial clone)
FROGGY_FORK_URL=../qmk_firmware ./build.sh                # clone from a local fork, no network
```

- `build.sh` owns `.cache/qmk_firmware` (marker `.git/froggy-cache`); it force-checks-out and `git clean -fdx` it every run. Do not work inside it.
- Build runs with `SKIP_GIT=yes` and no submodules; the nRF build does not use `lib/` submodule sources. Repeated `fatal: No names found, cannot describe anything.` lines come from `tmk_core/nrf.mk` and are harmless.
- `keymap/config.h` is included after the board `config.h`, so board values are overridden with `#undef` + `#define` (e.g. `RGBLED_NUM` / `RGBLED_NUM_DEFAULT` = 6). Per-effect `#undef RGBLIGHT_EFFECT_*` has no effect because the board config sets `RGBLIGHT_ANIMATIONS`.
- Toolchain: `arm-none-eabi-gcc`, `qmk` CLI, `python3`, `git`, `make`. Flashing = copy the `.uf2` to the BMP bootloader drive by hand; `nrfutil` does not work under WSL2.
- Code style: spaces for indentation (fork's `.clang-format`); scripts are bash with `set -euo pipefail`.

## Hardware facts

- **The active keymap is `KEYMAP.JSN` on the BMP drive, not the layers in `keymap.c`.** `keymap.c` still matters for custom keycodes (the `"RGBRST\0"...` name string table used by `keycode_str_converter` when parsing `KEYMAP.JSN`) and `process_record_user`. Keymap C files must include `bmp.h` / `bmp_custom_keycode.h`.
- Runtime config is `CONFIG.JSN`: `is_left_hand: 1`, locale JP, `led: {pin: 1, num: 6}`. LED data pin 1 = BMP `PIN1` = Pro Micro `D3` (Helix LED data pin). On BMP the config file's `led.pin` / `led.num` take precedence over `RGB_DI_PIN` / `RGBLED_NUM_DEFAULT`; check the config file first when debugging RGB.
- LED power comes from BMP pin 21 (USB 5V through a diode, ~4.3V), so LEDs can light only while USB is connected. On Helix-type PCBs pins 21 and 24 are shorted; this hardware issue keeps the LEDs dark (per the BMP README, the pin-24 header must be removed). Config and `RGB_TOG` (layer 2 in `KEYMAP.JSN`) are already correct.
- Layer 5 in `KEYMAP.JSN` is a mouse layer, toggled by `TG(5)` on the bottom-right key (reached via `MO(4)` from the base layer, and on layer 5 itself). `EX(CMB(...))` combos parse but are not implemented in this fork's extended-keycode processing, so use layer keys instead.
- **No OLED is mounted.**
- `JP_LT` / `JP_GT` (used in `KEYMAP.JSN` and `keymap.c`) come from the fork's customized `quantum/keymap_extras/keymap_jp.h`.

## USB in BIOS/UEFI

The normal build (HID + mass storage) works over a USB cable in BIOS (verified on one PC). F-keys such as `F2` are on layer 2, reached with `OSL(2)` from layer 4. BMP's USB HID stack is in the closed BMP API, so boot-protocol support cannot be checked from source. If a PC rejects the composite device, add `#define DISABLE_MSC 1` to `keymap/config.h` (keep the froggy keymap; do not switch to `no_msc`). That build hides the drive, so edit and back up JSN files **before** flashing it; flash the normal `.uf2` from the bootloader drive to get the drive back.

## BMP drive access from WSL2

The drive (Windows volume label `Helix/frogg`, the first 11 chars of `device_info.name`) is not auto-mounted under `/mnt`, and mounting needs sudo. Access it only through `powershell.exe` as `scripts/bmp-config.sh` does (`Copy-Item -LiteralPath`, not `Get-Content`, to keep bytes intact). **Read-only unless the user explicitly asks to restore or flash.** Never copy a `.uf2` to the drive or run `restore` on your own initiative.

## Design docs

- `docs/design.md`: current spec (layout contract, `build.sh` / `bmp-config.sh` interfaces, exit codes, acceptance criteria).
- `docs/adr/`: decisions and rationale (why a separate repo with a pinned fork).
