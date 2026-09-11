// Copyright 2026 Hajime Oh-yake (@Hajime Oh-yake)
// SPDX-License-Identifier: GPL-2.0-or-later
//
// HostOS QMK glue (tap-dance-slot design).
// Design: quantum/host_os/docs/host-os-design.md

#include "host_os.h"

#include "quantum.h"
#include "dynamic_keymap.h"
#include "keymap_introspection.h"
#include "os_detection.h"

#if !defined(VIAL_ENABLE) || !defined(TAP_DANCE_ENABLE)
#    error "HostOS requires VIAL_ENABLE and TAP_DANCE_ENABLE (it lives in Vial tap dance slots)"
#endif

/* Cast to int because these compare values from two different enums (-Werror=enum-compare) */
_Static_assert((int)HOST_OS_UNSURE == (int)OS_UNSURE, "HOST_OS_UNSURE does not match os_variant_t");
_Static_assert((int)HOST_OS_LINUX == (int)OS_LINUX, "HOST_OS_LINUX does not match os_variant_t");
_Static_assert((int)HOST_OS_WINDOWS == (int)OS_WINDOWS, "HOST_OS_WINDOWS does not match os_variant_t");
_Static_assert((int)HOST_OS_MACOS == (int)OS_MACOS, "HOST_OS_MACOS does not match os_variant_t");
_Static_assert((int)HOST_OS_IOS == (int)OS_IOS, "HOST_OS_IOS does not match os_variant_t");

/* The pure-logic side assumes these values for KC_NO / KC_TRNS and the tap dance range */
_Static_assert(HOST_OS_KC_NO == KC_NO, "HOST_OS_KC_NO does not match KC_NO");
_Static_assert(HOST_OS_KC_TRNS == KC_TRNS, "HOST_OS_KC_TRNS does not match KC_TRNS");
_Static_assert((int)HOST_OS_QK_TAP_DANCE == (int)QK_TAP_DANCE, "HOST_OS_QK_TAP_DANCE does not match QK_TAP_DANCE");
_Static_assert((int)HOST_OS_QK_TAP_DANCE_MAX == (int)QK_TAP_DANCE_MAX, "HOST_OS_QK_TAP_DANCE_MAX does not match QK_TAP_DANCE_MAX");

/* HostOS slots are taken from the end of the tap dance table, so the count must fit */
_Static_assert(HOST_OS_COUNT > 0, "HOST_OS_COUNT must be at least 1 (set it in rules.mk)");
_Static_assert(HOST_OS_COUNT <= VIAL_TAP_DANCE_ENTRIES, "HOST_OS_COUNT exceeds VIAL_TAP_DANCE_ENTRIES (not enough tap dance slots)");

/* Default when keymap.c does not define host_os_actions[] (all KC_NO).
   Weak, so a strong definition in keymap.c takes precedence. */
__attribute__((weak)) const host_os_entry_t host_os_actions[HOST_OS_COUNT] = {0};

/* Reinterpret a Vial tap dance entry as a HostOS entry (field order: tap, hold, double tap, tap+hold) */
static inline host_os_entry_t host_os_from_tap_dance(const vial_tap_dance_entry_t *td) {
    return (host_os_entry_t){td->on_tap, td->on_hold, td->on_double_tap, td->on_tap_hold};
}

uint16_t host_os_translate_keycode(uint16_t keycode) {
    /* A HostOS field may itself hold another HostOS key; resolve repeatedly, but bound the
       depth by the slot count so a reference cycle always terminates. */
    for (uint8_t depth = 0; depth < HOST_OS_COUNT; depth++) {
        uint8_t td_index;
        if (!host_os_index_of_keycode(keycode, VIAL_TAP_DANCE_ENTRIES, HOST_OS_COUNT, &td_index)) return keycode;

        vial_tap_dance_entry_t td;
        if (dynamic_keymap_get_tap_dance(td_index, &td) != 0) return KC_NO;

        host_os_entry_t entry = host_os_from_tap_dance(&td);
        keycode = host_os_select((uint8_t)detected_host_os(), &entry);
    }
    return KC_NO; /* unresolved after HOST_OS_COUNT hops: reference cycle */
}

/* Write host_os_actions[] into HostOS slots that are still completely empty.
 *
 * dynamic_keymap_reset() fills every tap dance slot with {KC_NO x4, TAPPING_TERM}. A slot
 * whose four keycodes are all KC_NO and whose custom_tapping_term is not our marker is
 * therefore "fresh" and receives the keymap.c default; the marker is written alongside so
 * the slot is never seeded twice. Slots the user has touched are left alone. */
void host_os_post_init(void) {
    for (uint8_t i = 0; i < HOST_OS_COUNT; i++) {
        uint8_t td_index = (uint8_t)(HOST_OS_BASE + i);
        vial_tap_dance_entry_t td;
        if (dynamic_keymap_get_tap_dance(td_index, &td) != 0) continue;
        if (td.custom_tapping_term == HOST_OS_SEEDED_MAGIC) continue;
        if (td.on_tap != KC_NO || td.on_hold != KC_NO || td.on_double_tap != KC_NO || td.on_tap_hold != KC_NO) continue;

        const host_os_entry_t *d = &host_os_actions[i];
        vial_tap_dance_entry_t seeded = {d->kc_macos, d->kc_windows, d->kc_linux, d->kc_default, HOST_OS_SEEDED_MAGIC};
        dynamic_keymap_set_tap_dance(td_index, &seeded);
    }
}

#ifndef HOST_OS_NO_POST_INIT_HOOK
/* Runs at the end of keyboard_init(), i.e. after via_init() has (possibly) reset the EEPROM.
   Keyboards that define keyboard_post_init_kb() themselves must define
   HOST_OS_NO_POST_INIT_HOOK and call host_os_post_init() from their own hook. */
void keyboard_post_init_kb(void) {
    host_os_post_init();
    keyboard_post_init_user();
}
#endif

/* Replaces the weak definition in quantum/keymap_common.c.
 *
 * Substituting here means both the tapping machinery (action_for_key) and
 * process_record (get_record_keycode) see the OS-specific keycode from the start.
 * As a result a mod-tap or layer-tap placed in a HostOS field is processed through
 * exactly the same path, with exactly the same latency, as one written directly in the keymap.
 * The VIAL_MATRIX_MAGIC branch must go through the translation too, otherwise a HostOS key
 * placed inside another tap dance's field would not resolve.
 *
 * Note: this duplicates the body of quantum/keymap_common.c:204.
 *       When updating vial-qmk, check that function for changes.
 *       Only one override of this function can exist per firmware. */
uint16_t keymap_key_to_keycode(uint8_t layer, keypos_t key) {
    /* Identical to the implementation in quantum/keymap_common.c --- begin */
    if (vial_unlock_in_progress) {
        return KC_NO;
    }
    uint16_t keycode = KC_NO;
    if (key.row == VIAL_MATRIX_MAGIC && key.col == VIAL_MATRIX_MAGIC) {
        keycode = g_vial_magic_keycode_override;
    } else if (key.row < MATRIX_ROWS && key.col < MATRIX_COLS) {
        keycode = keycode_at_keymap_location(layer, key.row, key.col);
    }
#ifdef ENCODER_MAP_ENABLE
    else if (key.row == KEYLOC_ENCODER_CW && key.col < NUM_ENCODERS) {
        keycode = keycode_at_encodermap_location(layer, key.col, true);
    } else if (key.row == KEYLOC_ENCODER_CCW && key.col < NUM_ENCODERS) {
        keycode = keycode_at_encodermap_location(layer, key.col, false);
    }
#endif // ENCODER_MAP_ENABLE
#ifdef DIP_SWITCH_MAP_ENABLE
    else if (key.row == KEYLOC_DIP_SWITCH_ON && key.col < NUM_DIP_SWITCHES) {
        keycode = keycode_at_dip_switch_map_location(key.col, true);
    } else if (key.row == KEYLOC_DIP_SWITCH_OFF && key.col < NUM_DIP_SWITCHES) {
        keycode = keycode_at_dip_switch_map_location(key.col, false);
    }
#endif // DIP_SWITCH_MAP_ENABLE
    /* --- end */

    return host_os_translate_keycode(keycode);
}
