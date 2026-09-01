/*
 * RISC-V implementation of the Talos architecture boundary (RV64, Sv39).
 *
 * Verified against the proven source tree in
 * examples/sel4-qemu-validation/sel4test:
 *   - CONFIG_PT_LEVELS == 3 (Sv39):   cbuild/kernel/gen_config/kernel/gen_config.h
 *   - seL4_RISCV_PageTableObject,
 *     seL4_RISCV_Default_VMAttributes: kernel/libsel4/arch_include/riscv/sel4/arch/types.h
 *   - seL4_RISCV_ASIDPool_Assign,
 *     seL4_RISCV_PageTable_{Map,Unmap},
 *     seL4_RISCV_Page_{Map,Unmap}:     kernel/libsel4/arch_include/riscv/interfaces/object-api-arch.xml
 *   - seL4_LargePageBits == 21:        seL4 manual sec. 7.1.2.6 (RISC-V 64-bit page sizes)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <autoconf.h> // CONFIG_PT_LEVELS
#include <utils/util.h> // BIT()

#include "talos/arch.h"

seL4_Word talos_arch_intermediate_levels(void)
{
    // The VSpace root is itself one of the CONFIG_PT_LEVELS levels.
    return CONFIG_PT_LEVELS - 1;
}

seL4_Word talos_arch_pagetable_type(seL4_Word level)
{
    // RISC-V uses the same object type at every level, unlike AArch64.
    (void) level;
    return seL4_RISCV_PageTableObject;
}

seL4_Word talos_arch_pagetable_size_bits(seL4_Word level)
{
    (void) level;
    return seL4_PageTableBits;
}

seL4_Word talos_arch_leaf_span(void)
{
    // A leaf table holds 512 4K entries, i.e. 2 MiB, which is exactly seL4's
    // large-page size on this architecture.
    return BIT(seL4_LargePageBits);
}

int talos_arch_asidpool_assign(seL4_CPtr pool, seL4_CPtr vspace)
{
    return seL4_RISCV_ASIDPool_Assign(pool, vspace);
}

int talos_arch_pagetable_map(seL4_CPtr pt, seL4_CPtr vspace, uintptr_t vaddr)
{
    return seL4_RISCV_PageTable_Map(pt, vspace, vaddr,
                                    seL4_RISCV_Default_VMAttributes);
}

int talos_arch_pagetable_unmap(seL4_CPtr pt)
{
    return seL4_RISCV_PageTable_Unmap(pt);
}

int talos_arch_page_map(seL4_CPtr frame, seL4_CPtr vspace, uintptr_t vaddr,
                        seL4_CapRights_t rights)
{
    return seL4_RISCV_Page_Map(frame, vspace, vaddr, rights,
                               seL4_RISCV_Default_VMAttributes);
}

int talos_arch_page_unmap(seL4_CPtr frame)
{
    return seL4_RISCV_Page_Unmap(frame);
}
