/*
 * Talos test rootserver.
 *
 * Same bootstrap as the product rootserver, then runs the test suites instead
 * of creating the production partition. Built only when configured with
 * -DTALOS_ROOTSERVER=tests, so no test code can reach a product image.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdio.h>
#include <sel4/sel4.h>
#include <utils/util.h> // ARRAY_SIZE()

#include "talos/boot.h"
#include "talos_test.h"

static talos_boot_t boot;

// Add new suites here as subsystems land. See tests/include/talos_test.h.
static const talos_test_suite_t *const suites[] = {
    &talos_suite_partition,
};

int main(void)
{
    printf("talOS test rootserver booting\n");

    if (talos_boot_init(&boot) != 0) {
        printf("talOS tests: failed to bootstrap allocator\n");
        printf("talOS tests: FAILURES PRESENT\n");
        seL4_TCB_Suspend(seL4_CapInitThreadTCB);
        return 1;
    }

    talos_test_env_t env = {
        .bootinfo = boot.bootinfo,
        .vka = &boot.vka,
        .allocman = boot.allocman,
    };

    int failed = talos_test_run(suites, ARRAY_SIZE(suites), &env);

    seL4_TCB_Suspend(seL4_CapInitThreadTCB);

    return failed;
}
