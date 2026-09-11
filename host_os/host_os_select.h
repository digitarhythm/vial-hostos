// Copyright 2026 Hajime Oh-yake (@Hajime Oh-yake)
// SPDX-License-Identifier: GPL-2.0-or-later
//
// HostOS pure logic.
// Has no QMK dependency, so it can be tested with the host gcc alone.
// Design: quantum/host_os/docs/host-os-design.md

#pragma once

#include <stdbool.h>
#include <stdint.h>

// Same values as QMK's os_variant_t.
// The match is verified at compile time by _Static_assert in host_os.c.
enum {
    HOST_OS_UNSURE  = 0,
    HOST_OS_LINUX   = 1,
    HOST_OS_WINDOWS = 2,
    HOST_OS_MACOS   = 3,
    HOST_OS_IOS     = 4,
};

// Keycodes treated as "not set" (KC_NO = 0, KC_TRNS = 1)
#define HOST_OS_KC_NO   0x0000
#define HOST_OS_KC_TRNS 0x0001

// One HostOS entry. It lives in a Vial tap dance slot; the four tap dance
// fields are reinterpreted as follows (same order as the Vial GUI shows them):
//   on_tap        -> kc_macos    (macOS and iOS)
//   on_hold       -> kc_windows
//   on_double_tap -> kc_linux    (ChromeOS lands here too)
//   on_tap_hold   -> kc_default  (unknown OS, or when the matching field is empty)
typedef struct {
    uint16_t kc_macos;
    uint16_t kc_windows;
    uint16_t kc_linux;
    uint16_t kc_default;
} host_os_entry_t;

// Marker written into the (otherwise unused) custom_tapping_term field of a
// HostOS slot once it has been seeded with the keymap.c defaults ("OS").
#define HOST_OS_SEEDED_MAGIC 0x4F53

// Pick the keycode for the detected OS.
//   os    : an os_variant_t value
//   entry : one entry (must not be NULL)
// A return value of HOST_OS_KC_NO means "send nothing".
uint16_t host_os_select(uint8_t os, const host_os_entry_t *entry);

// QMK tap dance keycode range (QK_TAP_DANCE .. QK_TAP_DANCE_MAX).
// The match is verified at compile time by _Static_assert in host_os.c.
#define HOST_OS_QK_TAP_DANCE     0x5700
#define HOST_OS_QK_TAP_DANCE_MAX 0x57FF

// First tap dance index used by HostOS: the last `count` slots of `td_entries`.
//   td_entries : VIAL_TAP_DANCE_ENTRIES
//   count      : HOST_OS_COUNT
// Returns td_entries (i.e. "no slots") when count is 0 or larger than td_entries.
uint8_t host_os_base(uint8_t td_entries, uint8_t count);

// True when tap dance slot `td_index` belongs to HostOS.
bool host_os_is_target_index(uint8_t td_index, uint8_t td_entries, uint8_t count);

// If keycode is TD(n) and slot n belongs to HostOS, store n in *td_index and return true.
//   keycode    : the keycode to test
//   td_entries : VIAL_TAP_DANCE_ENTRIES
//   count      : HOST_OS_COUNT
//   td_index   : output, the tap dance slot number (must not be NULL)
bool host_os_index_of_keycode(uint16_t keycode, uint8_t td_entries, uint8_t count, uint8_t *td_index);
