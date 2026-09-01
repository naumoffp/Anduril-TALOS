#!/bin/bash
#
# Runs INSIDE the seL4 build container. Not meant to be called directly -
# use scripts/test.sh from the repo root.
#
# Builds the test rootserver, boots it under QEMU, and decides pass/fail from
# the harness's own verdict line. Exits non-zero on failure, on a missing
# verdict (hang, early fault, kernel panic), or on a build error - so CI and
# #20's timing work can rely on the exit status.
#
# SPDX-License-Identifier: BSD-2-Clause
#
set -uo pipefail

TIMEOUT="${TALOS_TEST_TIMEOUT:-120}"
LOG=/host/sel4/talos/cbuild-tests/test-run.log

bash /host/scripts/_container-build.sh tests || exit 1

cd /host/sel4/talos/cbuild-tests || exit 1

echo "=== booting test rootserver (timeout ${TIMEOUT}s) ==="
# QEMU here has no reason to read stdin; detach it so a stalled boot cannot
# block forever waiting on a terminal.
timeout "$TIMEOUT" ./simulate </dev/null 2>&1 | sed -e 's/\r$//' | tee "$LOG"
rc=${PIPESTATUS[0]}

echo
echo "=== verdict ==="
if grep -q '^talOS tests: ALL PASSED' "$LOG"; then
    grep -E '^  (tests|checks):' "$LOG"
    echo "RESULT: PASS"
    exit 0
fi

if grep -q '^talOS tests: FAILURES PRESENT' "$LOG"; then
    echo "Failed checks:"
    grep -E '^\s+\[ FAIL \]' "$LOG" || true
    grep -E '^  (tests|checks):' "$LOG"
    echo "RESULT: FAIL (assertions failed)"
    exit 1
fi

# No verdict line at all: the run died before the harness finished.
if [ "$rc" -eq 124 ]; then
    echo "RESULT: FAIL (timed out after ${TIMEOUT}s, no verdict)"
else
    echo "RESULT: FAIL (no verdict line; QEMU exited rc=$rc)"
fi
echo "--- last 25 lines ---"
tail -25 "$LOG"
exit 1
