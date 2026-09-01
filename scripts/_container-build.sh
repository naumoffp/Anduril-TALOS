#!/bin/bash
#
# Runs INSIDE the seL4 build container. Not meant to be called directly -
# use scripts/build.sh or scripts/test.sh from the repo root.
#
# $1 = rootserver variant: "root" (product) or "tests"
#
# SPDX-License-Identifier: BSD-2-Clause
#
set -euo pipefail

VARIANT="${1:-root}"
PROJECT=/host/sel4/talos

case "$VARIANT" in
    root)  BUILD_DIR="$PROJECT/cbuild" ;;
    tests) BUILD_DIR="$PROJECT/cbuild-tests" ;;
    *) echo "unknown variant '$VARIANT' (expected root or tests)" >&2; exit 2 ;;
esac

# Configure only when the build directory is new. The seL4 CMake tool bakes
# absolute paths into build.ninja, so a configured directory is not portable -
# but it is reusable in place, which is what makes rebuilds fast.
if [ ! -f "$BUILD_DIR/build.ninja" ]; then
    echo "=== configuring $VARIANT rootserver in $BUILD_DIR ==="
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    ../init-build.sh \
        -DPLATFORM=qemu-riscv-virt \
        -DRISCV64=TRUE \
        -DSIMULATION=TRUE \
        -DTALOS_ROOTSERVER="$VARIANT"
else
    echo "=== reusing existing build directory $BUILD_DIR ==="
fi

cd "$BUILD_DIR"
echo "=== building ==="
ninja
