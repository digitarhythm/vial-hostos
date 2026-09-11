// Copyright 2026 Hajime Oh-yake (@Hajime Oh-yake)
// SPDX-License-Identifier: GPL-2.0-or-later
//
// HostOS QMK glue.
// The last HOST_OS_COUNT Vial tap dance slots are reinterpreted as HostOS keys:
// a HostOS key sends a different keycode depending on the detected host OS.
// No Vial protocol or EEPROM layout change is involved.
//
// Design: quantum/host_os/docs/host-os-design.md

#pragma once

#include <stdint.h>

#include "vial.h"             /* VIAL_TAP_DANCE_ENTRIES, vial_tap_dance_entry_t */
#include "quantum_keycodes.h" /* TD() */
#include "host_os_select.h"

// Number of tap dance slots (taken from the end) that act as HostOS keys.
// Set from rules.mk: HOST_OS_COUNT = 16
#ifndef HOST_OS_COUNT
#    define HOST_OS_COUNT 16
#endif

// First tap dance slot used by HostOS. With 32 slots and HOST_OS_COUNT 16: 0..15 are
// ordinary tap dances, 16..31 are HostOS keys.
#define HOST_OS_BASE (VIAL_TAP_DANCE_ENTRIES - HOST_OS_COUNT)

// HostOS key n (0 .. HOST_OS_COUNT-1). This is simply TD(HOST_OS_BASE + n).
#define HOS(n) TD(HOST_OS_BASE + (n))

// Type used from keymap.c (the pure-logic struct as is)
typedef host_os_entry_t host_os_action_t;

// Helper for writing one entry in keymap.c:
//   HOST_OS(.kc_macos = KC_LNG2, .kc_windows = KC_INT5, .kc_linux = LCTL(KC_SPC))
// Fields that are not given become 0 (KC_NO).
#define HOST_OS(...) ((host_os_entry_t){__VA_ARGS__})

// Optional defaults defined in keymap.c. After an EEPROM reset they are written into any
// HostOS slot that is still completely empty (see host_os_post_init()).
//   const host_os_entry_t host_os_actions[HOST_OS_COUNT] = {
//       [0] = HOST_OS(.kc_macos = KC_LGUI, .kc_windows = KC_LCTL),
//   };
// If keymap.c does not define it, the weak default in host_os.c (all KC_NO) is used.
extern const host_os_entry_t host_os_actions[HOST_OS_COUNT];

// Translate a HostOS tap dance keycode into the keycode for the current OS.
// Any other keycode is returned unchanged. Called from the keymap_key_to_keycode() override.
uint16_t host_os_translate_keycode(uint16_t keycode);

// Seed empty HostOS slots with host_os_actions[]. Called from keyboard_post_init_kb()
// unless HOST_OS_NO_POST_INIT_HOOK is defined, in which case the keyboard must call it.
void host_os_post_init(void);
