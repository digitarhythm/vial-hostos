# HostOS — Integration Guide (for keyboard authors)

How to add HostOS to an existing Vial-enabled keyboard in this tree.
Design rationale: `host-os-design.md`. GUI side: `host-os-protocol.md`.

---

## What HostOS is

HostOS detects the host operating system (macOS / Windows / Linux, iOS treated as macOS) and
lets **one key send a different keycode per OS** — for example `Cmd` on macOS and `Ctrl` on
Windows from the same physical key.

It works by reserving the **last N Vial tap dance slots** and reinterpreting their four fields
as macOS / Windows / Linux / Default. Nothing in Vial's protocol or EEPROM layout changes.

---

## Prerequisites

| Requirement | How to check |
|---|---|
| Vial-enabled firmware | `keymaps/<name>/rules.mk` has `VIAL_ENABLE = yes` |
| Tap dance enabled (Vial default) | `TAP_DANCE_ENABLE` is not set to `no` |
| The `host_os/` directory is in the tree | any location works; this tree keeps it at `quantum/host_os/`. On stock vial-qmk just copy the directory in |
| Enough tap dance slots | `HOST_OS_COUNT` ≤ `VIAL_TAP_DANCE_ENTRIES` (32 on a 4 KB-EEPROM board; the build stops otherwise) |

OS detection is switched on automatically.

---

## Steps

### 1. Put the count in `vial.json`

In `keymaps/<name>/vial.json`:

```json
{
  "hostOS": {"count": 16},
  ...
}
```

This single value serves both the Vial GUI (which reads it from the keyboard definition) and
the firmware build (which reads it from the same file). With 32 tap dance slots, 16 leaves
`TD(0)`–`TD(15)` as ordinary tap dances and turns slots 16–31 into `HOS(0)`–`HOS(15)`.

### 1b. Include `host_os.mk` from the keymap's `rules.mk`

In `keymaps/<name>/rules.mk` (the file that already has `VIAL_ENABLE = yes`, next to `vial.json`):

```make
include quantum/host_os/host_os.mk     # path from the repository root
```

### 2. Include the library in `keymap.c`

```c
#include QMK_KEYBOARD_H
#include "host_os.h"
```

### 3. Optional: defaults in `keymap.c`

```c
const host_os_entry_t host_os_actions[HOST_OS_COUNT] = {
    [0] = HOST_OS(.kc_macos = KC_LGUI, .kc_windows = KC_LCTL),
    [1] = HOST_OS(.kc_macos = KC_LNG2, .kc_windows = KC_INT5, .kc_linux = LCTL(KC_SPC)),
};
```

| Field | OS |
|---|---|
| `.kc_macos` | macOS and iOS |
| `.kc_windows` | Windows |
| `.kc_linux` | Linux (ChromeOS too) |
| `.kc_default` | unknown OS, or when the matching field is empty |

Omitted fields are empty. The defaults are written into a HostOS slot **only while that slot is
completely empty** (right after an EEPROM reset); anything you set from the Vial GUI wins and is
never overwritten. Skip this step to start with all slots empty and configure everything from
the GUI.

### 4. Use `HOS(n)` in the keymap

```c
[_BASE] = LAYOUT(
    ...,
    HOS(0),    // <- HostOS entry 0 (= TD(16) with 32 slots / 16 entries)
    ...
),
```

In the Vial GUI the same key appears as `HOS(0)` (HostOS-aware GUI) or `TD(16)` (stock GUI).

### 5. Build and flash

```bash
make <keyboard>:<keymap>
```

Flashing resets the EEPROM; reload your `.vil` afterwards. HostOS entries are stored inside the
tap dance section of the `.vil`, so no special handling is needed.

---

## Choosing the count

| Slots | `hostOS.count` | Ordinary tap dances left |
|---:|---:|---:|
| 32 | 16 (default) | 16 |
| 32 | 8 | 24 |
| 32 | 32 | 0 |
| 16 | 4 | 12 |

A count of 0 (or a missing key) is rejected by `host_os.mk`; a count larger than the slot count is rejected by a static assert.

---

## Interaction with existing code

### You use `process_detected_host_os_user()` to switch layers

It keeps working alongside HostOS, but usually becomes unnecessary; removing it frees the
duplicated per-OS layers.

### You define `keymap_key_to_keycode()` yourself

HostOS defines the same function, so the link fails. Merge your logic into the version in
`host_os.c`, or do not enable HostOS.

### You define `keyboard_post_init_kb()` yourself

Add to `config.h`:

```c
#define HOST_OS_NO_POST_INIT_HOOK
```

and call `host_os_post_init();` from your own `keyboard_post_init_kb()`.

### You already use the last tap dance slots

They become HostOS keys. Move those tap dances to lower slot numbers, or lower
`hostOS.count` in `vial.json`.

---

## If the build fails

| Message | Cause | Fix |
|---|---|---|
| `HOST_OS_COUNT exceeds VIAL_TAP_DANCE_ENTRIES` | more HostOS slots than tap dance slots | lower `hostOS.count` in `vial.json` |
| `HostOS: .../vial.json must contain "hostOS": {"count": N} with N >= 1` | the key is missing, unreadable, or 0 | add `"hostOS": {"count": N}` to `vial.json` |
| `HostOS requires VIAL_ENABLE and TAP_DANCE_ENABLE` | not a Vial build, or tap dance disabled | enable them |
| `multiple definition of 'keymap_key_to_keycode'` / `'keyboard_post_init_kb'` | you define the same function | see above |
| `host_os.h: No such file or directory` | the `include .../host_os.mk` line is missing or its path is wrong | check the keymap `rules.mk` |

---

## Limitations

| # | Note |
|---|---|
| 1 | The reserved slots cannot be used as ordinary tap dances |
| 2 | For the first tens of milliseconds after USB enumeration the OS is unknown and Default is used |
| 3 | ChromeOS is detected as Linux; put the ChromeOS keycode in the Linux field |
| 4 | `HOS(n)` has no multi-tap / hold behaviour of its own. Put a mod-tap keycode such as `LGUI_T(KC_TAB)` in a field if you need one |
| 5 | `HOS(n)` may be nested inside tap dance fields, key overrides and macros |
| 6 | A stock Vial GUI shows the slots in its Tap Dance tab (labels Tap / Hold / Double Tap / Tap+Hold mean macOS / Windows / Linux / Default). A HostOS-aware GUI shows a HostOS tab instead |

---

## Minimal example

```make
# keyboards/<kb>/keymaps/vial/rules.mk
VIAL_ENABLE = yes
include quantum/host_os/host_os.mk
```

```json
// keyboards/<kb>/keymaps/vial/vial.json (excerpt)
{ "hostOS": {"count": 16}, ... }
```

```c
// keyboards/<kb>/keymaps/vial/keymap.c
#include QMK_KEYBOARD_H
#include "host_os.h"

const host_os_entry_t host_os_actions[HOST_OS_COUNT] = {
    [0] = HOST_OS(.kc_macos = KC_LGUI, .kc_windows = KC_LCTL),
};

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT( HOS(0), KC_A, KC_B, ... ),
};
```

`HOS(0)` now sends `Cmd` on macOS and `Ctrl` on Windows.
