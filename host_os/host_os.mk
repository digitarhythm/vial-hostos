# HostOS — drop-in build integration.
#
# 1. Put the slot count in the keymap's vial.json (the Vial GUI reads it from there):
#
#        "hostOS": {"count": 16}
#
# 2. Include this file from the *keymap-level* rules.mk (the one next to vial.json):
#
#        include quantum/host_os/host_os.mk      # path relative to the repository root
#
# That is all: the count is taken from vial.json, so it is defined in exactly one place.
# The last <count> tap dance slots become HostOS keys. Nothing in Vial's core sources,
# build files, protocol or EEPROM layout is touched; this file only adds sources and defines.
# Design: docs/host-os-design.md next to this file.

# Directory of this .mk (works wherever the host_os/ directory is placed)
HOST_OS_DIR := $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST))))
# Directory of the rules.mk that included us = the keymap directory holding vial.json
HOST_OS_KEYMAP_DIR := $(patsubst %/,%,$(dir $(lastword $(filter-out $(lastword $(MAKEFILE_LIST)),$(MAKEFILE_LIST)))))
HOST_OS_VIAL_JSON := $(HOST_OS_KEYMAP_DIR)/vial.json

# Single source of truth: "hostOS": {"count": N} in vial.json
HOST_OS_COUNT := $(shell python3 -c 'import json,sys; d=json.load(open(sys.argv[1])); print(d.get("hostOS",{}).get("count","missing"))' $(HOST_OS_VIAL_JSON) 2>/dev/null || echo unreadable)
ifeq ($(filter-out 0 missing unreadable,$(HOST_OS_COUNT)),)
    $(error HostOS: $(HOST_OS_VIAL_JSON) must contain "hostOS": {"count": N} with N >= 1 (found: $(HOST_OS_COUNT)))
endif

OS_DETECTION_ENABLE = yes
VPATH += $(HOST_OS_DIR)
SRC   += host_os.c host_os_select.c
OPT_DEFS += -DVIAL_HOST_OS_ENABLE -DHOST_OS_COUNT=$(HOST_OS_COUNT)
