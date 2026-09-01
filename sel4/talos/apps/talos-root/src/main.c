/*
 * Talos product rootserver.
 *
 * Boots, brings up the allocator, creates partition 0, and parks. All the
 * real work lives in libs/libtalos; this file only sequences it. The test
 * rootserver is apps/talos-tests and shares the same bootstrap.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdio.h>
#include <sel4/sel4.h>

#include "talos/boot.h"
#include "talos/partition.h"

// Placeholder CSpace size for one partition: 2^4 = 16 slots. Revisit once
// partition process counts are configurable (#7).
#define PARTITION_CNODE_SIZE_BITS 4

// Virtual address the partition's paging structures are built for.
// Placeholder: it only gives the intermediate page tables a concrete address
// to populate. The real value falls out of wherever the partition ELF is
// loaded (#10).
#define PARTITION_VADDR_BASE 0x10000000

static talos_boot_t boot;
static partition_t partition0;

// Root task never returns to a caller, so on failure report and park.
static void fail_if(int error, const char *msg)
{
    if (error) {
        printf("talOS: %s (error %d)\n", msg, error);
        seL4_TCB_Suspend(seL4_CapInitThreadTCB);
    }
}

// elfloader boots the kernel, the kernel boots the root task with PC/SP set,
// sel4runtime's crt0 runs, then main().
int main(void)
{
    printf("talOS root task booting\n");

    fail_if(talos_boot_init(&boot), "failed to bootstrap allocator");
    talos_boot_print(&boot);
    printf("talOS: BootInfo read complete.\n");

    printf("talOS: creating partition 0\n");
    int error = partition_create(&partition0, 0, PARTITION_CNODE_SIZE_BITS,
                                 PARTITION_VADDR_BASE, &boot.vka,
                                 boot.allocman, boot.bootinfo);
    // Print before bailing out: a partial failure still has useful provenance.
    partition_print(&partition0, boot.bootinfo);
    fail_if(error, "failed to create partition 0");

    printf("talOS: partition 0 CSpace + VSpace created, idling.\n");

    // Nothing to run yet: no TCB (#5, #7), no partition image (#10).
    seL4_TCB_Suspend(seL4_CapInitThreadTCB);

    return 0;
}
