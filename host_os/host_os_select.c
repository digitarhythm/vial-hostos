// Copyright 2026 Hajime Oh-yake (@Hajime Oh-yake)
// SPDX-License-Identifier: GPL-2.0-or-later
//
// HostOS pure logic.
// Has no QMK dependency, so it can be tested with the host gcc alone.
// Design: quantum/host_os/docs/host-os-design.md

#include "host_os_select.h"

// KC_NO and KC_TRNS are both treated as "not set"
static inline bool host_os_unset(uint16_t kc) {
    return (kc == HOST_OS_KC_NO || kc == HOST_OS_KC_TRNS);
}

uint16_t host_os_select(uint8_t os, const host_os_entry_t *entry) {
    uint16_t kc;

    switch (os) {
        case HOST_OS_MACOS:
        case HOST_OS_IOS: // iOS behaves like macOS
            kc = entry->kc_macos;
            break;
        case HOST_OS_WINDOWS:
            kc = entry->kc_windows;
            break;
        case HOST_OS_LINUX:
            kc = entry->kc_linux;
            break;
        case HOST_OS_UNSURE:
        default:
            kc = entry->kc_default;
            break;
    }

    // Fall back to the Default field when the selected field is not set
    if (host_os_unset(kc)) kc = entry->kc_default;

    // If Default is not set either, send nothing
    if (host_os_unset(kc)) kc = HOST_OS_KC_NO;

    return kc;
}

uint8_t host_os_base(uint8_t td_entries, uint8_t count) {
    // count 0 (feature off) or more slots than exist -> no HostOS slots at all
    if (count == 0 || count > td_entries) return td_entries;
    return (uint8_t)(td_entries - count);
}

bool host_os_is_target_index(uint8_t td_index, uint8_t td_entries, uint8_t count) {
    if (td_index >= td_entries) return false;
    return td_index >= host_os_base(td_entries, count);
}

bool host_os_index_of_keycode(uint16_t keycode, uint8_t td_entries, uint8_t count, uint8_t *td_index) {
    if (keycode < HOST_OS_QK_TAP_DANCE || keycode > HOST_OS_QK_TAP_DANCE_MAX) return false;

    uint8_t idx = (uint8_t)(keycode & 0xFF);
    if (!host_os_is_target_index(idx, td_entries, count)) return false;

    *td_index = idx;
    return true;
}
