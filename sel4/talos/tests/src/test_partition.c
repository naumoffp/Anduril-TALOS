/*
 * Partition suite: issues #6 (VSpace + CSpace from Untyped) and #3 (CSpace).
 *
 * The structural tests check what #6 asks for. The behavioural tests use
 * documented seL4 error codes as oracles, so they prove the objects are
 * actually usable rather than merely allocated without complaint:
 *
 *   - A further PageTable_Map at vaddr_base must return seL4_DeleteFirst,
 *     because "if all paging structures and mappings are present for this
 *     virtual address, return an seL4_DeleteFirst error" (manual 10.10.6.1).
 *     That positively proves every level down to the leaf is populated.
 *
 *   - A 4K Page_Map at vaddr_base must succeed. Page_Map returns
 *     seL4_FailedLookup when the vspace "does not have a paging structure at
 *     the required level mapped at vaddr. Or, vspace is not assigned to an
 *     ASID pool" (manual 10.10.5.2), so success proves the paging chain and
 *     the ASID assignment together.
 *
 *   - The same map one leaf-span higher must fail with seL4_FailedLookup,
 *     proving we built exactly one leaf and did not cover more address space
 *     than intended.
 *
 * Each test builds its own partition so tests stay independent. Those objects
 * are not freed: this is a test rootserver that parks when done, and the
 * handful of objects involved is far below what QEMU reports as available.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdio.h>
#include <utils/util.h> // BIT(), ARRAY_SIZE()

#include "talos_test.h"
#include "talos/partition.h"
#include "talos/arch.h"

#define TEST_CNODE_SIZE_BITS 4
#define TEST_VADDR_BASE      0x10000000

// Builds a partition for one test. Returns false (having recorded a failed
// check) if construction itself failed, so the caller can bail out.
static bool make_partition(talos_test_env_t *env, partition_t *p)
{
    int error = partition_create(p, 0, TEST_CNODE_SIZE_BITS, TEST_VADDR_BASE,
                                 env->vka, env->allocman, env->bootinfo);

    CHECK(error == 0, "partition_create succeeded (error %d)", error);
    return error == 0;
}

static void test_provenance(talos_test_env_t *env)
{
    partition_t p;
    if (!make_partition(env, &p)) {
        return;
    }

    seL4_BootInfo *bi = env->bootinfo;
    seL4_Word num_untyped = bi->untyped.end - bi->untyped.start;
    seL4_Word expected = 2 + talos_arch_intermediate_levels();

    CHECK(p.num_objects == expected, "built %lu objects, expected %lu",
          (unsigned long) p.num_objects, (unsigned long) expected);

    for (seL4_Word i = 0; i < p.num_objects; i++) {
        const partition_object_t *rec = &p.objects[i];

        CHECK(rec->untyped_index < num_untyped,
              "%s resolved to an untyped (index %lu)",
              rec->label, (unsigned long) rec->untyped_index);
        if (rec->untyped_index >= num_untyped) {
            continue;
        }

        seL4_UntypedDesc *desc = &bi->untypedList[rec->untyped_index];
        uintptr_t base = (uintptr_t) desc->paddr;
        seL4_Word span = BIT(desc->sizeBits);

        // #6 requires non-device Untyped. vka_alloc_* passes
        // can_use_dev = false so this holds by construction - assert it
        // anyway, because it is the isolation property that matters.
        CHECK(desc->isDevice == 0, "%s came from non-device untyped[%lu]",
              rec->label, (unsigned long) rec->untyped_index);

        CHECK(rec->paddr >= base && (rec->paddr - base) + rec->bytes <= span,
              "%s [0x%lx,0x%lx) fits inside untyped[%lu] [0x%lx,0x%lx)",
              rec->label,
              (unsigned long) rec->paddr,
              (unsigned long) (rec->paddr + rec->bytes),
              (unsigned long) rec->untyped_index,
              (unsigned long) base,
              (unsigned long) (base + span));

        CHECK((rec->paddr % rec->bytes) == 0,
              "%s paddr 0x%lx is %lu-byte aligned",
              rec->label, (unsigned long) rec->paddr,
              (unsigned long) rec->bytes);
    }
}

static void test_disjoint(talos_test_env_t *env)
{
    partition_t p;
    if (!make_partition(env, &p)) {
        return;
    }

    // Spatial isolation starts here. The same check applied across partitions
    // is what ARINC 653 spatial partitioning rests on (#6, later #23).
    for (seL4_Word i = 0; i < p.num_objects; i++) {
        for (seL4_Word j = i + 1; j < p.num_objects; j++) {
            const partition_object_t *a = &p.objects[i];
            const partition_object_t *b = &p.objects[j];
            bool disjoint = (a->paddr + a->bytes <= b->paddr)
                            || (b->paddr + b->bytes <= a->paddr);

            CHECK(disjoint, "%s and %s do not overlap (0x%lx+%lu vs 0x%lx+%lu)",
                  a->label, b->label,
                  (unsigned long) a->paddr, (unsigned long) a->bytes,
                  (unsigned long) b->paddr, (unsigned long) b->bytes);
        }
    }
}

static void test_paging_complete(talos_test_env_t *env)
{
    partition_t p;
    if (!make_partition(env, &p)) {
        return;
    }

    vka_object_t probe;
    int error = vka_alloc_object(env->vka, talos_arch_pagetable_type(0),
                                 talos_arch_pagetable_size_bits(0), &probe);
    if (error) {
        CHECK(false, "allocated a probe page table (error %d)", error);
        return;
    }

    error = talos_arch_pagetable_map(probe.cptr, p.vspace_root, p.vaddr_base);
    CHECK_ERR(error, seL4_DeleteFirst, "extra PageTable_Map at vaddr_base");

    if (error == seL4_NoError) {
        // It mapped after all, so a level was missing. Leave no trace.
        talos_arch_pagetable_unmap(probe.cptr);
    }
    vka_free_object(env->vka, &probe);
}

static void test_frame_mapping(talos_test_env_t *env)
{
    partition_t p;
    if (!make_partition(env, &p)) {
        return;
    }

    vka_object_t frame;
    int error = vka_alloc_frame(env->vka, seL4_PageBits, &frame);
    if (error) {
        CHECK(false, "allocated a test frame (error %d)", error);
        return;
    }

    error = talos_arch_page_map(frame.cptr, p.vspace_root, p.vaddr_base,
                                seL4_AllRights);
    CHECK_ERR(error, seL4_NoError, "4K frame maps at vaddr_base");
    if (error == seL4_NoError) {
        talos_arch_page_unmap(frame.cptr);
    }

    uintptr_t beyond = p.vaddr_base + talos_arch_leaf_span();
    error = talos_arch_page_map(frame.cptr, p.vspace_root, beyond,
                                seL4_AllRights);
    CHECK_ERR(error, seL4_FailedLookup, "4K frame one leaf-span higher");
    if (error == seL4_NoError) {
        talos_arch_page_unmap(frame.cptr);
    }

    vka_free_object(env->vka, &frame);
}

static void test_cspace_slots(talos_test_env_t *env)
{
    partition_t p;
    if (!make_partition(env, &p)) {
        return;
    }

    // The CNode is empty and nothing refers to it yet, but it must already
    // work as a capability table. Copy a cap into the first and last slot,
    // then delete both. Giving it a guard so a thread can resolve CPtrs
    // through it as a CSpace root is the remainder of #3.
    seL4_Word slots[] = { 0, BIT(p.cspace_size_bits) - 1 };

    for (unsigned i = 0; i < ARRAY_SIZE(slots); i++) {
        int error = seL4_CNode_Copy(p.cspace_root, slots[i],
                                    p.cspace_size_bits,
                                    seL4_CapInitThreadCNode, p.vspace_root,
                                    seL4_WordBits, seL4_AllRights);
        CHECK(error == seL4_NoError,
              "CSpace accepts a cap in slot %lu of %lu (error %d)",
              (unsigned long) slots[i],
              (unsigned long) BIT(p.cspace_size_bits), error);

        if (error == seL4_NoError) {
            error = seL4_CNode_Delete(p.cspace_root, slots[i],
                                      p.cspace_size_bits);
            CHECK(error == seL4_NoError, "CSpace slot %lu cleaned up (error %d)",
                  (unsigned long) slots[i], error);
        }
    }
}

static const talos_test_t partition_tests[] = {
    { "partition.provenance",      "#6", test_provenance },
    { "partition.disjoint",        "#6", test_disjoint },
    { "partition.paging_complete", "#6", test_paging_complete },
    { "partition.frame_mapping",   "#6", test_frame_mapping },
    { "partition.cspace_slots",    "#3", test_cspace_slots },
};

const talos_test_suite_t talos_suite_partition = {
    .name = "partition",
    .tests = partition_tests,
    .num_tests = ARRAY_SIZE(partition_tests),
};
