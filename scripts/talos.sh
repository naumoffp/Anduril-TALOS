#!/bin/bash
#
# Talos build / test / run driver. Run from anywhere; paths are resolved from
# this script's location.
#
# Everything runs inside the seL4 build container from
# examples/sel4-qemu-validation/seL4-docker, via that Makefile's own EXEC hook,
# so this stays consistent with how the container is meant to be driven.
#
# The whole repo root is mounted at /host (not just sel4/), because `repo`
# needs to see .git to clone the manifest from the local checkout.
#
# SPDX-License-Identifier: BSD-2-Clause
#
set -euo pipefail

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DOCKER_MK="$REPO/examples/sel4-qemu-validation/seL4-docker"

usage() {
    cat <<'EOF'
usage: scripts/talos.sh <command>

  build [root|tests]   configure if needed, then build (default: root)
  test                 build the test rootserver, boot it, and exit non-zero
                       unless every check passes
  run                  boot the product rootserver (Ctrl-A then X to exit)
  shell                interactive shell in the build container
EOF
    exit 2
}

# Run a command in the container. Setting EXEC makes the Makefile drop its
# interactive flags, so pass want_tty=1 for anything that needs a terminal.
# docker refuses -it without a real tty, so only ask for it when we have one -
# that keeps `run` usable from a pipe, a script or CI.
# `user_run` rather than `user` skips the container image rebuild, which is
# only needed the first time.
in_container() {
    local want_tty="$1"; shift
    local -a flags=()
    if [ "$want_tty" = "1" ] && [ -t 0 ] && [ -t 1 ]; then
        flags=(DOCKER_RUN_FLAGS=-it)
    fi
    make -s -C "$DOCKER_MK" user_run HOST_DIR="$REPO" "${flags[@]}" EXEC="$1"
}

cmd="${1:-}"
case "$cmd" in
    build)
        in_container 0 "bash /host/scripts/_container-build.sh ${2:-root}"
        ;;
    test)
        in_container 0 "bash /host/scripts/_container-test.sh"
        ;;
    run)
        in_container 0 "bash /host/scripts/_container-build.sh root"
        echo "=== booting product rootserver (Ctrl-A then X to exit) ==="
        in_container 1 "bash -c 'cd /host/sel4/talos/cbuild && ./simulate'"
        ;;
    shell)
        make -C "$DOCKER_MK" user HOST_DIR="$REPO"
        ;;
    *)
        usage
        ;;
esac
