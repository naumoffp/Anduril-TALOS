#
# Talos top-level build toggles.
#
# Adapted from examples/sel4-qemu-validation/sel4test's proven
# easy-settings.cmake, trimmed to what Phase 1 needs. Removed: sel4test's
# own test-runner options (LibSel4TestPrinterRegex,
# LibSel4TestPrinterHaltOnTestFailure, BAMBOO) and the DOMAINS/SMP/
# NUM_NODES/MCS/ARM_HYP toggles - not used until later phases (see
# settings.cmake).
#
# Usage (or just use scripts/build.sh and scripts/test.sh from the repo root):
#   mkdir cbuild && cd cbuild
#   ../init-build.sh -DPLATFORM=qemu-riscv-virt -DRISCV64=TRUE -DSIMULATION=TRUE
#   ninja
#   ./simulate
#
# SPDX-License-Identifier: BSD-2-Clause
#

set(SIMULATION OFF CACHE BOOL "Build for QEMU simulation")
set(RELEASE OFF CACHE BOOL "Performance optimized build")
set(VERIFICATION OFF CACHE BOOL "Only verification friendly kernel features")
set(PLATFORM "qemu-riscv-virt" CACHE STRING "Platform to build for")
set(KernelSel4Arch "" CACHE STRING "aarch32, aarch64, arm_hyp, ia32, x86_64, riscv32, riscv64")

# Which rootserver this build produces. "root" is the product image;
# "tests" swaps in apps/talos-tests, which runs the suites under tests/.
# Separate build directories (cbuild/ and cbuild-tests/) keep both available.
set(TALOS_ROOTSERVER "root" CACHE STRING "Rootserver to build: root or tests")
set_property(CACHE TALOS_ROOTSERVER PROPERTY STRINGS "root;tests")
