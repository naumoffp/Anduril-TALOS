#
# Talos seL4 project.
#
# Adapted from examples/sel4-qemu-validation/sel4test's top-level
# CMakeLists.txt (projects/sel4test/CMakeLists.txt), which we proved boots on
# qemu-riscv-virt / RISCV64=TRUE.
#
# Layout:
#   libs/libtalos   Talos's own code, one file per subsystem
#   tests/          test harness + one suite per subsystem (libtalostest)
#   apps/talos-root product rootserver
#   apps/talos-tests test rootserver, links libtalostest
#
# Exactly one rootserver is declared per build, chosen by TALOS_ROOTSERVER
# (see easy-settings.cmake). This mirrors how sel4test ships a single
# rootserver, and keeps test code out of product images entirely.
#
# SPDX-License-Identifier: BSD-2-Clause
#

cmake_minimum_required(VERSION 3.16.0)

include(settings.cmake)

project(talos C CXX ASM)

find_package(seL4 REQUIRED)
find_package(elfloader-tool REQUIRED)
find_package(musllibc REQUIRED)
find_package(util_libs REQUIRED)
find_package(seL4_libs REQUIRED)
find_package(sel4_projects_libs REQUIRED)

# Root task CNode size: large enough for DTB, timer caps, etc.
# Matches sel4test's proven value for this same platform.
set(KernelRootCNodeSizeBits 13 CACHE INTERNAL "")

sel4_import_kernel()

if(KernelArchRiscV)
    ApplyData61ElfLoaderSettings(${KernelPlatform} ${KernelSel4Arch})
endif()
elfloader_import_project()

# Shared build environment for every app and library below. Done once here
# rather than per-app, since both rootservers need it.
set(UserLinkerGCSections OFF CACHE BOOL "" FORCE)
musllibc_setup_build_environment_with_sel4runtime()
sel4_import_libsel4()
util_libs_import_libraries()
sel4_libs_import_libraries()
sel4_projects_libs_import_libraries()

add_subdirectory(libs/libtalos)
add_subdirectory(tests)
add_subdirectory(apps/talos-root)
add_subdirectory(apps/talos-tests)

include(rootserver)
if(TALOS_ROOTSERVER STREQUAL "tests")
    message(STATUS "Talos: building the TEST rootserver (apps/talos-tests)")
    DeclareRootserver(talos-tests)
elseif(TALOS_ROOTSERVER STREQUAL "root")
    message(STATUS "Talos: building the product rootserver (apps/talos-root)")
    DeclareRootserver(talos-root)
else()
    message(FATAL_ERROR "TALOS_ROOTSERVER must be 'root' or 'tests', got '${TALOS_ROOTSERVER}'")
endif()

if(SIMULATION)
    include(simulation)
    GenerateSimulateScript()
endif()
