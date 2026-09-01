/*
 * Talos partition state and construction. Issues #3, #6.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdint.h>
#include <sel4/sel4.h>
#include <vka/vka.h>
#include <vka/object.h>
#include <allocman/allocman.h>

// Deepest paging hierarchy Talos is prepared to build: RV64 Sv39 and Sv48 need
// 3 and 4, AArch64 needs 4. This only bounds the objects[] array below; the
// number actually created comes from talos_arch_intermediate_levels().
#define PARTITION_MAX_PT_LEVELS 4

// One CNode + one VSpace root + up to (PARTITION_MAX_PT_LEVELS - 1)
// intermediate page tables.
#define PARTITION_MAX_OBJECTS (2 + PARTITION_MAX_PT_LEVELS - 1)

// Provenance of one kernel object retyped for a partition.
//
// untyped_cookie is allocman's opaque cookie and is what vka_free_object()
// needs - it is NOT a CPtr, so it cannot be used to index bi->untypedList[].
// untyped_index is that index, recovered by physical address instead.
typedef struct {
    const char *label;
    seL4_CPtr cptr;
    seL4_Word untyped_cookie;
    seL4_Word untyped_index;
    uintptr_t paddr;
    seL4_Word bytes;          // actual size of the object in bytes
} partition_object_t;

// Talos's per-partition kernel state.
//
// The shape follows the PARTITION_KERNEL_STATE schema of the ARINC 653 formal
// specification (arXiv:2206.14189, Appendix A.4), which pairs exactly one
// KernelPageTableRef with each partition and holds the partition's memory
// alongside it. The schema's remaining fields arrive with later issues:
//   ProcessTable                      -> #5, #7  (process.h)
//   ReadyQueue / WaitingQueue         -> #7, #8  (pm.h)
//   TimeCounterQueue                  -> #13, #16 (time.h)
//   Periodicity, OperatingMode,
//   StartCondition, LockLevel         -> #23     (partition modes)
typedef struct {
    seL4_Word id;
    seL4_CPtr cspace_root;            // partition CNode (ARINC: partition CSpace)
    seL4_Word cspace_size_bits;       // slot_bits, so 2^n slots
    seL4_CPtr vspace_root;            // ARINC: KernelPageTableRef
    uintptr_t vaddr_base;             // vaddr the paging structures were built for
    partition_object_t objects[PARTITION_MAX_OBJECTS];
    seL4_Word num_objects;
} partition_t;

// Retypes non-device Untyped into the partition's CNode, VSpace root and the
// intermediate page tables needed to reach a 4K-mappable leaf at vaddr_base,
// and assigns the VSpace an ASID.
//
// Returns 0 on success, or the first non-zero seL4/vka error encountered. On
// failure *p is left partially populated, with num_objects reflecting whatever
// was allocated, so the caller can still report provenance.
int partition_create(partition_t *p, seL4_Word id, seL4_Word cspace_size_bits,
                     uintptr_t vaddr_base, vka_t *vka, allocman_t *allocman,
                     seL4_BootInfo *bi);

// Prints each object and the BootInfo untyped it was carved from.
void partition_print(const partition_t *p, seL4_BootInfo *bi);

// Recovers which bi->untypedList[] entry contains paddr, or the untyped count
// if none does. Untyped regions do not overlap, so the first match is the only
// match.
seL4_Word partition_untyped_index_for_paddr(seL4_BootInfo *bi, uintptr_t paddr);
