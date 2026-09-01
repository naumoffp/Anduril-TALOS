#
# libtalos: Talos's own kernel-object plumbing, independent of which
# rootserver links it.
#
# Subsystems land here as the backlog progresses. Current and planned:
#   boot.c       root-task bootstrap                        (done)
#   partition.c  partition VSpace + CSpace, modes           (#3, #6, #23)
#   process.c    process control blocks, TCB pool           (#5, #14, #15)
#   pm.c         Partition Manager thread + APEX dispatch   (#7, #8, #17, #19)
#   time.c       system clock, time-counter queue, releases (#11, #12, #13, #16)
#   schedule.c   major frame + time windows, domains        (#22, #24)
#
# SPDX-License-Identifier: BSD-2-Clause
#

cmake_minimum_required(VERSION 3.16.0)

# Talos is RISC-V only for now. The architecture-dependent surface is confined
# to src/arch/<arch>/ behind include/talos/arch.h - see that header for the
# measured porting cost to AArch64 (sponsor challenge 1).
if(KernelArchRiscV)
    set(talos_arch_srcs src/arch/riscv/paging.c)
else()
    message(
        FATAL_ERROR
            "Talos currently supports RISC-V only (KernelArchRiscV). "
            "Porting to ${KernelArch} means implementing include/talos/arch.h "
            "under libs/libtalos/src/arch/${KernelArch}/ - see that header."
    )
endif()

file(GLOB talos_srcs src/*.c)

add_library(talos STATIC EXCLUDE_FROM_ALL ${talos_srcs} ${talos_arch_srcs})

target_include_directories(talos PUBLIC include)
target_link_libraries(
    talos
    PUBLIC
        sel4_autoconf
        muslc
        sel4
        sel4runtime
        sel4allocman
        sel4vka
        sel4utils
        sel4simple
        sel4simple-default
        sel4platsupport
)
target_compile_options(talos PRIVATE -Werror -g)
