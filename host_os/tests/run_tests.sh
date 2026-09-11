#!/bin/sh
# Host-side unit tests for HostOS
# Run: quantum/host_os/tests/run_tests.sh
set -e

DIR=$(cd "$(dirname "$0")" && pwd)
OUT="${TMPDIR:-/tmp}/host_os_test"

cc -std=c11 -Wall -Wextra -Werror -o "$OUT" \
    "$DIR/host_os_test.c" \
    "$DIR/../host_os_select.c"

"$OUT"
