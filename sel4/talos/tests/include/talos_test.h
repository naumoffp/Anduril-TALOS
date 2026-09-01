/*
 * Talos test harness.
 *
 * Tests are built into their own rootserver (apps/talos-tests), never into the
 * product rootserver (apps/talos-root), so no test code ships. Select with
 *   ../init-build.sh -DTALOS_ROOTSERVER=tests ...
 *
 * Adding a suite for a new subsystem:
 *   1. tests/src/test_<subsystem>.c defines its tests and one
 *      talos_test_suite_t, named e.g. talos_suite_process.
 *   2. Declare it in this header's "suites" block below.
 *   3. Add it to the array in apps/talos-tests/src/main.c.
 * The file is picked up by the glob in tests/CMakeLists.txt automatically.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <sel4/sel4.h>
#include <vka/vka.h>
#include <allocman/allocman.h>

// Everything a test needs to build real kernel objects. Tests run in the test
// rootserver, so they get the genuine BootInfo and a live allocator.
typedef struct {
    seL4_BootInfo *bootinfo;
    vka_t *vka;
    allocman_t *allocman;
} talos_test_env_t;

typedef struct {
    const char *name;         // "subsystem.behaviour"
    const char *issue;        // GitHub issue this covers, e.g. "#6"
    void (*fn)(talos_test_env_t *env);
} talos_test_t;

typedef struct {
    const char *name;
    const talos_test_t *tests;
    size_t num_tests;
} talos_test_suite_t;

// Records one assertion. Prefer the CHECK macro.
void talos_test_check(bool passed, const char *fmt, ...);

// Runs every test in every suite and prints a summary.
// Returns the total number of failed checks; 0 means everything passed.
int talos_test_run(const talos_test_suite_t *const *suites, size_t num_suites,
                   talos_test_env_t *env);

#define CHECK(cond, fmt, ...) talos_test_check((cond), fmt, ##__VA_ARGS__)

// Convenience for the common "a call returned the error code we expected" case,
// which is how most Talos behaviour is proven: seL4 documents an exact error
// for each failure mode, so the code is the oracle.
#define CHECK_ERR(actual, expected, what)                                   \
    talos_test_check((actual) == (expected),                                \
                     "%s: got %d, expected %s=%d",                          \
                     (what), (int) (actual), #expected, (int) (expected))

// Suites. One per subsystem; see the header comment for how to add one.
extern const talos_test_suite_t talos_suite_partition;   // #3, #6
