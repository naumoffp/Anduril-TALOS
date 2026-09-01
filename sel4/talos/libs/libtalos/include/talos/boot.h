/*
 * Talos root-task bootstrap: BootInfo, allocator, vka.
 *
 * Shared by both rootservers (apps/talos-root and apps/talos-tests) so the
 * bring-up sequence exists in exactly one place.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <sel4/sel4.h>
#include <simple/simple.h>
#include <vka/vka.h>
#include <allocman/allocman.h>

typedef struct {
    seL4_BootInfo *bootinfo;
    simple_t simple;
    allocman_t *allocman;
    vka_t vka;
} talos_boot_t;

// Reads BootInfo and brings up an allocman-backed vka over the Untyped the
// kernel handed us. Returns 0 on success, non-zero if the allocator could not
// be bootstrapped.
//
// This is the same sequence sel4test's own root task uses, down to the
// bookkeeping pool size:
//   simple_default_init_bootinfo -> bootstrap_use_current_simple
//                                -> allocman_make_vka
int talos_boot_init(talos_boot_t *boot);

// Prints node info and the full Untyped list (the Phase 1 "Hello seL4" output).
void talos_boot_print(const talos_boot_t *boot);
