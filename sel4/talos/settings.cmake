#
# Talos build settings: wires CMAKE_MODULE_PATH to the repo-synced kernel,
# seL4 CMake helpers, elfloader-tool, and every projects/* subproject.
#
# Adapted from examples/sel4-qemu-validation/sel4test's proven
# projects/sel4test/settings.cmake, trimmed to what Phase 1 ("Hello seL4",
# brief sec.8 step 1) needs: a single-domain, single-core, non-MCS kernel.
# Removed: sel4test's test-harness-only options
# (Sel4testAllowSettingsOverride, Sel4testHaveCache, Sel4testSimulation,
# BAMBOO / LibSel4TestPrintXML) and the DOMAINS/SMP/MCS build toggles -
# those become relevant starting brief sec.8 step 5 (multi-partition +
# domains) and should be added back deliberately then, not carried as
# unused logic now.
#
# SPDX-License-Identifier: BSD-2-Clause
#

cmake_minimum_required(VERSION 3.16.0)

set(project_dir "${CMAKE_CURRENT_LIST_DIR}")
file(GLOB project_modules ${project_dir}/projects/*)
list(APPEND CMAKE_MODULE_PATH ${project_dir}/kernel ${project_dir}/tools/seL4/cmake-tool/helpers/
     ${project_dir}/tools/seL4/elfloader-tool/ ${project_modules})

set(NANOPB_SRC_ROOT_FOLDER "${project_dir}/tools/nanopb" CACHE STRING "NanoPB Folder location")
set(OPENSBI_PATH "${project_dir}/tools/opensbi" CACHE STRING "OpenSBI Folder location")

set(SEL4_CONFIG_DEFAULT_ADVANCED ON)

include(application_settings)

include(${CMAKE_CURRENT_LIST_DIR}/easy-settings.cmake)

correct_platform_strings()

find_package(seL4 REQUIRED)
sel4_configure_platform_settings()

set(valid_platforms ${KernelPlatform_all_strings} ${correct_platform_strings_platform_aliases})
set_property(CACHE PLATFORM PROPERTY STRINGS ${valid_platforms})
if(NOT "${PLATFORM}" IN_LIST valid_platforms)
  message(FATAL_ERROR "Invalid PLATFORM selected: \"${PLATFORM}\"
Valid platforms are: \"${valid_platforms}\"")
endif()

if(KernelPlatformQEMUArmVirt OR KernelPlatformQEMURiscVVirt OR KernelPlatformSpike)
  set(SIMULATION ON CACHE BOOL "" FORCE)
endif()

if(SIMULATION)
  ApplyCommonSimulationSettings(${KernelSel4Arch})
endif()

ApplyCommonReleaseVerificationSettings(${RELEASE} ${VERIFICATION})

# Fixed for Phase 1: single domain, single core, no scheduling contexts.
# Revisit when brief sec.8 step 5 (multi-partition + domains) begins.
set(KernelNumDomains 1 CACHE STRING "" FORCE)
set(KernelMaxNumNodes 1 CACHE STRING "" FORCE)
set(KernelIsMCS OFF CACHE BOOL "" FORCE)
