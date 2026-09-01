/*
 * Talos architecture boundary.
 *
 * seL4 has no architecture-neutral spelling for ASID assignment or paging
 * invocations: they are seL4_RISCV_*, seL4_ARM_*, seL4_X86_* and so on. This
 * header is the single seam where that architecture dependence lives, so the
 * rest of libtalos stays arch-neutral.
 *
 * Talos currently supports RISC-V only (RV64 / Sv39). The seam exists because
 * an Arm Cortex-A port is sponsor challenge 1, and because we measured the
 * cost: an AArch64 build of the pre-refactor code configured cleanly but
 * failed with 13 compile errors across exactly these 8 symbols
 *   CONFIG_PT_LEVELS, seL4_RISCV_PageTableObject,
 *   seL4_RISCV_Default_VMAttributes, seL4_RISCV_ASIDPool_Assign,
 *   seL4_RISCV_PageTable_{Map,Unmap}, seL4_RISCV_Page_{Map,Unmap}
 * all of which are now behind this interface.
 *
 * Porting note: CONFIG_PT_LEVELS is the one that is not a rename. RISC-V has
 * a level *count* and one PageTable object type for every level. AArch64 has
 * no such count - it has a distinct object type per level
 * (PageGlobalDirectory / PageUpperDirectory / PageDirectory / PageTable) -
 * which is why talos_arch_pagetable_type() takes a level argument even though
 * the RISC-V implementation ignores it.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdint.h>
#include <sel4/sel4.h>

// Number of intermediate page tables that must be created below the VSpace
// root to reach a 4K-mappable leaf. RV64 Sv39 has 3 levels, the root is one of
// them, so this is 2.
seL4_Word talos_arch_intermediate_levels(void);

// seL4 object type and size for the intermediate page table at `level`, where
// level 0 is the one directly below the VSpace root.
seL4_Word talos_arch_pagetable_type(seL4_Word level);
seL4_Word talos_arch_pagetable_size_bits(seL4_Word level);

// Virtual address span covered by a single leaf page table (2 MiB on Sv39).
// Used to reason about how much address space a partition's paging structures
// actually cover.
seL4_Word talos_arch_leaf_span(void);

// A VSpace is unusable by a thread until assigned an ASID (seL4 manual 7.1.4).
int talos_arch_asidpool_assign(seL4_CPtr pool, seL4_CPtr vspace);

// Installs `pt` at whichever level is still unpopulated for `vaddr`, starting
// from the VSpace root (seL4 manual 10.10.6.1). Returns seL4_DeleteFirst once
// every level for that vaddr is already present.
int talos_arch_pagetable_map(seL4_CPtr pt, seL4_CPtr vspace, uintptr_t vaddr);
int talos_arch_pagetable_unmap(seL4_CPtr pt);

// Maps a frame. Returns seL4_FailedLookup if the paging structures for `vaddr`
// are absent, or if `vspace` has no ASID (seL4 manual 10.10.5.2).
int talos_arch_page_map(seL4_CPtr frame, seL4_CPtr vspace, uintptr_t vaddr,
                        seL4_CapRights_t rights);
int talos_arch_page_unmap(seL4_CPtr frame);
