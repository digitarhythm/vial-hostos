# HostOS — Change Log by File

Date: 2026-09-08 (tap-dance-slot design; supersedes the dedicated-EEPROM revision of 2026-09-04)

## Modified

| File | Change |
|---|---|
| *(none)* | **No Vial file is modified at all** — `git diff -- builddefs quantum` is empty. Build integration lives in `quantum/host_os/host_os.mk`, included from each keymap's `rules.mk` |

**No Vial core file is modified.** The dedicated-EEPROM revision's edits to `quantum/vial.h`,
`vial.c`, `dynamic_keymap.h/.c`, `nvm/nvm_dynamic_keymap.h`, `nvm/eeprom/nvm_dynamic_keymap.c`,
`quantum_keycodes.h` and `quantum.h` were all reverted; `git diff -- quantum` is empty apart from
the untracked `quantum/host_os/` directory.

## New: `quantum/host_os/`

| File | Content |
|---|---|
| `host_os_select.h` | `host_os_entry_t` (4 × `uint16_t`: macOS, Windows, Linux, Default), `HOST_OS_SEEDED_MAGIC`, `host_os_select()`, `host_os_base()`, `host_os_is_target_index()`, `host_os_index_of_keycode()` |
| `host_os_select.c` | Pure logic. Selection with fallback (iOS → macOS field; empty → Default; Default empty → `KC_NO`); "last N of M slots" arithmetic; TD keycode → slot |
| `host_os.mk` | Drop-in build integration: self-locating, reads `hostOS.count` from the includer's `vial.json` (single source of truth), sets `OS_DETECTION_ENABLE`, `VPATH`/`SRC` and defines; errors if the key is missing or 0 |
| `host_os.h` | `HOST_OS_COUNT` default 16, `HOST_OS_BASE`, `HOS(n) = TD(HOST_OS_BASE + n)`, `HOST_OS(...)`, `host_os_actions[]`, `host_os_translate_keycode()`, `host_os_post_init()` |
| `host_os.c` | Static asserts; weak `host_os_actions[]`; translation (bounded nested resolution); seeding of empty slots with the marker; `keyboard_post_init_kb()` (opt-out `HOST_OS_NO_POST_INIT_HOOK`) and `keymap_key_to_keycode()` overrides |
| `tests/host_os_test.c`, `tests/run_tests.sh` | 54 host-side checks |
| `docs/` | design, GUI specification, integration guide, this file |

## Keyboards (`keyboards/trek/`)

| File | Change |
|---|---|
| `*/keymaps/*/rules.mk` (11 keymaps) | one line: `include quantum/host_os/host_os.mk` (keyboard-level `rules.mk` no longer mentions HostOS) |
| `*/keymaps/*/vial.json` (11) | `"hostOS": {"count": 16}` |
| `lettio/keymaps/default/keymap.c` | `#include "host_os.h"`, `host_os_actions[HOST_OS_COUNT]` default (`HOS(0)` = Cmd / Ctrl) |
| `lettio/config.h` | EEPROM map comment restored to the stock layout (macros 1891–4095) |
| `*/readme.md` | Firmware links unchanged; text already says 16 entries, `HOS(0)`–`HOS(15)` |

## Verification

| Item | Result |
|---|---|
| Host tests | 54 / 54 passed |
| `HOST_OS_BASE` / `HOS(0)` / `HOS(15)` on Lettio (32 slots) | 16 / `TD(16)` / `TD(31)` (compile-time probe) |
| Definition JSON | `hostOS.count = 16` read back from the compressed definition embedded in every enabled firmware |
| Builds | see the build log in the session report |
