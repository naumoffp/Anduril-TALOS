/*
 * Talos root-task bootstrap.
 *
 * API usage verified against the proven source tree in
 * examples/sel4-qemu-validation/sel4test:
 *   - sel4runtime_bootinfo():        projects/sel4runtime/include/sel4runtime.h
 *   - seL4_BootInfo layout:          kernel/libsel4/include/sel4/bootinfo_types.h
 *   - simple_default_init_bootinfo:  projects/seL4_libs/libsel4simple-default
 *   - bootstrap_use_current_simple,
 *     allocman_make_vka:             projects/seL4_libs/libsel4allocman
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdio.h>
#include <sel4runtime.h>

#include <simple-default/simple-default.h>
#include <allocman/bootstrap.h>
#include <allocman/vka.h>

#include "talos/boot.h"

// Static pool for the allocator's own bookkeeping (what is free, what is
// allocated). Same static byte array and size sel4test-driver uses for the
// same purpose.
#define ALLOCATOR_STATIC_POOL_SIZE ((1 << seL4_PageBits) * 20)
static char allocator_mem_pool[ALLOCATOR_STATIC_POOL_SIZE];

int talos_boot_init(talos_boot_t *boot)
{
    boot->bootinfo = sel4runtime_bootinfo();

    simple_default_init_bootinfo(&boot->simple, boot->bootinfo);

    boot->allocman = bootstrap_use_current_simple(
        &boot->simple, ALLOCATOR_STATIC_POOL_SIZE, allocator_mem_pool);
    if (boot->allocman == NULL) {
        return -1;
    }

    allocman_make_vka(&boot->vka, boot->allocman);

    return 0;
}

void talos_boot_print(const talos_boot_t *boot)
{
    seL4_BootInfo *bi = boot->bootinfo;
    seL4_Word num_untyped = bi->untyped.end - bi->untyped.start;

    printf("  node %lu of %lu\n",
           (unsigned long) bi->nodeID, (unsigned long) bi->numNodes);
    printf("  initial CNode size: 2^%lu slots\n",
           (unsigned long) bi->initThreadCNodeSizeBits);
    printf("  untyped regions: %lu\n", (unsigned long) num_untyped);

    for (seL4_Word i = 0; i < num_untyped; i++) {
        seL4_UntypedDesc *desc = &bi->untypedList[i];
        printf("  untyped[%lu]: paddr=0x%lx sizeBits=%u isDevice=%u\n",
               (unsigned long) i,
               (unsigned long) desc->paddr,
               (unsigned) desc->sizeBits,
               (unsigned) desc->isDevice);
    }
}
