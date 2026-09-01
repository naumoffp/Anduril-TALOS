/*
 * Talos partition construction: retype Untyped into one partition's CSpace
 * and VSpace. Issues #3, #6.
 *
 * Architecture-neutral: every ASID and paging invocation goes through
 * talos/arch.h. See that header for the port boundary.
 *
 * API usage verified against the proven source tree in
 * examples/sel4-qemu-validation/sel4test:
 *   - allocman_utspace_paddr:        projects/seL4_libs/libsel4allocman/include/allocman/allocman.h
 *   - vka_alloc_cnode_object, vka_alloc_object,
 *     vka_object_t.{cptr,ut,size_bits}:
 *                                    projects/seL4_libs/libsel4vka/include/vka/object.h
 *   - vka_alloc_vspace_root:         projects/seL4_libs/libsel4vka/arch_include/riscv
 *   - seL4_UntypedDesc.{paddr,sizeBits,isDevice}:
 *                                    kernel/libsel4/include/sel4/bootinfo_types.h
 *   - seL4_CapInitThreadASIDPool:    kernel/libsel4/include/sel4/bootinfo_types.h
 *   - seL4_SlotBits, BIT():          kernel/libsel4/include/sel4/, util_libs/libutils
 *
 * Non-device Untyped: vka_alloc_object / vka_alloc_cnode_object /
 * vka_alloc_vspace_root take no "allow device" flag - only
 * vka_alloc_frame_maybe_device does, as a deliberately separate opt-in path.
 * So nothing here can draw from device Untyped by construction; the test suite
 * asserts it regardless.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdio.h>
#include <utils/util.h> // BIT()

#include "talos/partition.h"
#include "talos/arch.h"

seL4_Word partition_untyped_index_for_paddr(seL4_BootInfo *bi, uintptr_t paddr)
{
    seL4_Word num_untyped = bi->untyped.end - bi->untyped.start;

    for (seL4_Word i = 0; i < num_untyped; i++) {
        seL4_UntypedDesc *desc = &bi->untypedList[i];
        uintptr_t base = (uintptr_t) desc->paddr;

        if (paddr >= base && paddr - base < BIT(desc->sizeBits)) {
            return i;
        }
    }

    return num_untyped;
}

// Records obj as belonging to p. `bytes` is the object's real size, which the
// caller knows from the object type: a CNode is 2^(slot_bits + seL4_SlotBits)
// bytes, whereas vka_object_t.size_bits holds only slot_bits.
static void partition_record(partition_t *p, allocman_t *allocman,
                             seL4_BootInfo *bi, const char *label,
                             vka_object_t *obj, seL4_Word bytes)
{
    if (p->num_objects >= PARTITION_MAX_OBJECTS) {
        // Caller bug, not a runtime condition: PARTITION_MAX_OBJECTS must
        // cover whatever partition_create builds on this architecture.
        printf("  partition_record: objects[] full, dropping %s\n", label);
        return;
    }

    partition_object_t *rec = &p->objects[p->num_objects++];

    rec->label = label;
    rec->cptr = obj->cptr;
    rec->untyped_cookie = obj->ut;
    // vka_object_t.ut is an opaque allocator cookie, not a CPtr: libsel4allocman's
    // split utspace returns the address of its own bookkeeping node for it
    // ("return the node as a cookie", src/utspace/split.c). So the originating
    // untyped has to be recovered by physical address. _utspace_split_paddr
    // ignores size_bits, but pass the object's own size anyway so this stays
    // correct against the documented interface.
    rec->paddr = allocman_utspace_paddr(allocman, obj->ut, obj->size_bits);
    rec->untyped_index = partition_untyped_index_for_paddr(bi, rec->paddr);
    rec->bytes = bytes;
}

// Builds the intermediate page tables down to a 4K-mappable leaf.
//
// Every Map call targets the same vspace + vaddr, because the kernel installs
// each table "at any unpopulated level for the provided virtual address"
// (seL4 manual sec. 10.10.6.1): the first call fills the level below the root,
// the next fills the one below that, and so on to the leaf.
static int partition_map_paging_structures(partition_t *p, vka_t *vka,
                                           allocman_t *allocman,
                                           seL4_BootInfo *bi)
{
    seL4_Word levels = talos_arch_intermediate_levels();

    for (seL4_Word level = 0; level < levels; level++) {
        seL4_Word size_bits = talos_arch_pagetable_size_bits(level);
        vka_object_t page_table;

        int error = vka_alloc_object(vka, talos_arch_pagetable_type(level),
                                     size_bits, &page_table);
        if (error) {
            return error;
        }
        partition_record(p, allocman, bi, "partition page table", &page_table,
                         BIT(size_bits));

        error = talos_arch_pagetable_map(page_table.cptr, p->vspace_root,
                                         p->vaddr_base);
        if (error != seL4_NoError) {
            return error;
        }
    }

    return 0;
}

int partition_create(partition_t *p, seL4_Word id, seL4_Word cspace_size_bits,
                     uintptr_t vaddr_base, vka_t *vka, allocman_t *allocman,
                     seL4_BootInfo *bi)
{
    *p = (partition_t) {
        .id = id,
        .cspace_size_bits = cspace_size_bits,
        .vaddr_base = vaddr_base,
    };

    // An empty capability table of 2^cspace_size_bits slots. Populating it and
    // giving it a guard so it works as a CSpace root is issue #3.
    vka_object_t cnode;
    int error = vka_alloc_cnode_object(vka, cspace_size_bits, &cnode);
    if (error) {
        return error;
    }
    p->cspace_root = cnode.cptr;
    partition_record(p, allocman, bi, "partition CNode", &cnode,
                     BIT(cspace_size_bits + seL4_SlotBits));

    // The top-level page table: the root of what becomes the partition's
    // private view of memory.
    vka_object_t vspace;
    error = vka_alloc_vspace_root(vka, &vspace);
    if (error) {
        return error;
    }
    p->vspace_root = vspace.cptr;
    partition_record(p, allocman, bi, "partition VSpace root", &vspace,
                     BIT(talos_arch_pagetable_size_bits(0)));

    error = talos_arch_asidpool_assign(seL4_CapInitThreadASIDPool,
                                       p->vspace_root);
    if (error != seL4_NoError) {
        return error;
    }

    return partition_map_paging_structures(p, vka, allocman, bi);
}

void partition_print(const partition_t *p, seL4_BootInfo *bi)
{
    seL4_Word num_untyped = bi->untyped.end - bi->untyped.start;

    printf("partition %lu: cspace=%lu (2^%lu slots) vspace=%lu vaddr_base=0x%lx\n",
           (unsigned long) p->id,
           (unsigned long) p->cspace_root,
           (unsigned long) p->cspace_size_bits,
           (unsigned long) p->vspace_root,
           (unsigned long) p->vaddr_base);

    for (seL4_Word i = 0; i < p->num_objects; i++) {
        const partition_object_t *rec = &p->objects[i];

        if (rec->untyped_index >= num_untyped) {
            printf("  %s: cptr=%lu paddr=0x%lx bytes=%lu (no matching untyped!)\n",
                   rec->label,
                   (unsigned long) rec->cptr,
                   (unsigned long) rec->paddr,
                   (unsigned long) rec->bytes);
            continue;
        }

        seL4_UntypedDesc *desc = &bi->untypedList[rec->untyped_index];
        printf("  %s: cptr=%lu paddr=0x%lx bytes=%lu from untyped[%lu] "
               "(paddr=0x%lx sizeBits=%u isDevice=%u)\n",
               rec->label,
               (unsigned long) rec->cptr,
               (unsigned long) rec->paddr,
               (unsigned long) rec->bytes,
               (unsigned long) rec->untyped_index,
               (unsigned long) desc->paddr,
               (unsigned) desc->sizeBits,
               (unsigned) desc->isDevice);
    }
}
