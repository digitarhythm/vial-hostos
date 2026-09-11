// Copyright 2026 Hajime Oh-yake (@Hajime Oh-yake)
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Host-side unit tests for the HostOS pure logic (tap-dance-slot design).
// Run: quantum/host_os/tests/run_tests.sh
// Design: quantum/host_os/docs/host-os-design.md

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

#include "../host_os_select.h"

#define KC_A 0x0004
#define KC_B 0x0005
#define TD(n) (0x5700 + (n))

#define MAC 0x0011
#define WIN 0x0022
#define LNX 0x0033
#define DEF 0x0044
#define NO   HOST_OS_KC_NO
#define TRNS HOST_OS_KC_TRNS

static int g_failed = 0, g_total = 0;
static void check(const char *name, uint16_t expected, uint16_t actual) {
    g_total++;
    if (expected == actual) printf("  ok   %-56s (0x%04X)\n", name, actual);
    else { g_failed++; printf("  FAIL %-56s expected 0x%04X, got 0x%04X\n", name, expected, actual); }
}
static uint16_t sel(uint8_t os, uint16_t mac, uint16_t win, uint16_t lnx, uint16_t def) {
    host_os_entry_t e = {mac, win, lnx, def}; return host_os_select(os, &e);
}

int main(void) {
    printf("struct layout\n");
    // 1: four uint16_t fields in tap dance field order (tap, hold, double tap, tap+hold)
    check("1  sizeof(host_os_entry_t) == 8", 8, (uint16_t)sizeof(host_os_entry_t));
    check("1  offset kc_macos   (on_tap)",        0, (uint16_t)offsetof(host_os_entry_t, kc_macos));
    check("1  offset kc_windows (on_hold)",       2, (uint16_t)offsetof(host_os_entry_t, kc_windows));
    check("1  offset kc_linux   (on_double_tap)", 4, (uint16_t)offsetof(host_os_entry_t, kc_linux));
    check("1  offset kc_default (on_tap_hold)",   6, (uint16_t)offsetof(host_os_entry_t, kc_default));
    check("1  HOST_OS_SEEDED_MAGIC == 'OS'", 0x4F53, HOST_OS_SEEDED_MAGIC);

    printf("\nhost_os_select()\n");
    // 2-6: per-OS selection; iOS uses the macOS field
    check("2  macOS   -> kc_macos",   MAC, sel(HOST_OS_MACOS,   MAC, WIN, LNX, DEF));
    check("3  iOS     -> kc_macos",   MAC, sel(HOST_OS_IOS,     MAC, WIN, LNX, DEF));
    check("4  Windows -> kc_windows", WIN, sel(HOST_OS_WINDOWS, MAC, WIN, LNX, DEF));
    check("5  Linux   -> kc_linux",   LNX, sel(HOST_OS_LINUX,   MAC, WIN, LNX, DEF));
    check("6  unsure  -> kc_default", DEF, sel(HOST_OS_UNSURE,  MAC, WIN, LNX, DEF));
    check("6  unknown -> kc_default", DEF, sel(255,             MAC, WIN, LNX, DEF));
    // 7-9: empty field falls back to Default; KC_TRNS counts as empty
    check("7  macOS empty -> Default",   DEF, sel(HOST_OS_MACOS,   NO,  WIN, LNX, DEF));
    check("7  Windows empty -> Default", DEF, sel(HOST_OS_WINDOWS, MAC, NO,  LNX, DEF));
    check("7  Linux empty -> Default",   DEF, sel(HOST_OS_LINUX,   MAC, WIN, NO,  DEF));
    check("8  Linux TRNS -> Default",    DEF, sel(HOST_OS_LINUX,   MAC, WIN, TRNS, DEF));
    check("9  iOS with macOS empty -> Default", DEF, sel(HOST_OS_IOS, NO, WIN, LNX, DEF));
    // 10: nothing is sent when Default is empty as well
    check("10 Linux and Default empty", NO, sel(HOST_OS_LINUX, MAC, WIN, NO, NO));
    check("10 Default is TRNS",         NO, sel(HOST_OS_LINUX, MAC, WIN, NO, TRNS));
    check("10 every field empty",       NO, sel(HOST_OS_MACOS, NO, NO, NO, NO));
    // 11: only Default set -> Default for every OS
    check("11 Default only / macOS",   DEF, sel(HOST_OS_MACOS,   NO, NO, NO, DEF));
    check("11 Default only / Windows", DEF, sel(HOST_OS_WINDOWS, NO, NO, NO, DEF));
    check("11 Default only / Linux",   DEF, sel(HOST_OS_LINUX,   NO, NO, NO, DEF));
    // 12: real keycodes
    check("12 real keycode / macOS",   KC_A, sel(HOST_OS_MACOS,   KC_A, KC_B, KC_B, KC_B));
    check("12 real keycode / Windows", KC_B, sel(HOST_OS_WINDOWS, KC_A, KC_B, KC_B, KC_B));

    printf("\nhost_os_base()\n");
    // 13: last `count` of `td_entries` (the user's example: 32 slots, 16 HostOS -> base 16)
    check("13 32 slots, count 16 -> base 16", 16, host_os_base(32, 16));
    check("13 32 slots, count 10 -> base 22", 22, host_os_base(32, 10));
    check("13 32 slots, count 32 -> base 0",   0, host_os_base(32, 32));
    check("13 16 slots, count 4  -> base 12", 12, host_os_base(16, 4));
    // 14: disabled or misconfigured -> no slots (base == td_entries)
    check("14 count 0 -> base == entries",        32, host_os_base(32, 0));
    check("14 count > entries -> base == entries", 8, host_os_base(8, 16));

    printf("\nhost_os_is_target_index()\n");
    // 15: 32 slots / 16 HostOS: 0..15 tap dance, 16..31 HostOS
    check("15 index 15 is tap dance",  0, host_os_is_target_index(15, 32, 16));
    check("15 index 16 is HostOS",     1, host_os_is_target_index(16, 32, 16));
    check("15 index 31 is HostOS",     1, host_os_is_target_index(31, 32, 16));
    check("15 index 32 out of range",  0, host_os_is_target_index(32, 32, 16));
    // 16: count 0 disables everything; count > entries disables everything
    check("16 count 0 / index 31",     0, host_os_is_target_index(31, 32, 0));
    check("16 count 40 / index 0",     0, host_os_is_target_index(0, 32, 40));

    printf("\nhost_os_index_of_keycode()\n");
    {
        uint8_t idx;
        // 17: TD keycodes inside the HostOS range return the slot number
        idx = 0xFF; check("17 TD(16) is HostOS",  1, host_os_index_of_keycode(TD(16), 32, 16, &idx)); check("17 slot", 16, idx);
        idx = 0xFF; check("17 TD(31) is HostOS",  1, host_os_index_of_keycode(TD(31), 32, 16, &idx)); check("17 slot", 31, idx);
        // 18: TD keycodes below the base stay ordinary tap dances
        idx = 0xFF; check("18 TD(0) is tap dance",  0, host_os_index_of_keycode(TD(0), 32, 16, &idx));
        check("18 TD(15) is tap dance",             0, host_os_index_of_keycode(TD(15), 32, 16, &idx));
        check("18 output untouched",             0xFF, idx);
        // 19: TD beyond the slot count, and non-TD keycodes
        check("19 TD(32) out of range",   0, host_os_index_of_keycode(TD(32), 32, 16, &idx));
        check("19 KC_A",                  0, host_os_index_of_keycode(KC_A, 32, 16, &idx));
        check("19 KC_NO",                 0, host_os_index_of_keycode(NO, 32, 16, &idx));
        check("19 0x56FF (just below)",   0, host_os_index_of_keycode(0x56FF, 32, 16, &idx));
        check("19 0x5800 (just above)",   0, host_os_index_of_keycode(0x5800, 32, 16, &idx));
        check("19 LGUI_T(KC_TAB)",        0, host_os_index_of_keycode(0x2CE2, 32, 16, &idx));
        // 20: other slot counts follow the "last N" rule
        idx = 0xFF; check("20 16 slots / count 4: TD(12) is HostOS", 1, host_os_index_of_keycode(TD(12), 16, 4, &idx)); check("20 slot", 12, idx);
        check("20 16 slots / count 4: TD(11) is tap dance", 0, host_os_index_of_keycode(TD(11), 16, 4, &idx));
        // 21: count 0 -> nothing is HostOS
        check("21 count 0: TD(31)", 0, host_os_index_of_keycode(TD(31), 32, 0, &idx));
    }

    printf("\n%d / %d passed", g_total - g_failed, g_total);
    if (g_failed) { printf("  (%d FAILED)\n", g_failed); return 1; }
    printf("\n"); return 0;
}
