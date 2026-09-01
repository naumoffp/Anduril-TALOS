/*
 * Talos test harness implementation.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdarg.h>
#include <stdio.h>

#include "talos_test.h"

static int checks_run;
static int checks_failed;

void talos_test_check(bool passed, const char *fmt, ...)
{
    va_list args;

    checks_run++;
    if (!passed) {
        checks_failed++;
    }

    printf("    [ %s ] ", passed ? "PASS" : "FAIL");
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
}

int talos_test_run(const talos_test_suite_t *const *suites, size_t num_suites,
                   talos_test_env_t *env)
{
    int tests_run = 0;
    int tests_failed = 0;

    checks_run = 0;
    checks_failed = 0;

    printf("\n==== talOS test run ====\n");

    for (size_t s = 0; s < num_suites; s++) {
        const talos_test_suite_t *suite = suites[s];

        printf("\nsuite: %s\n", suite->name);

        for (size_t t = 0; t < suite->num_tests; t++) {
            const talos_test_t *test = &suite->tests[t];
            int failed_before = checks_failed;

            printf("  %s (%s)\n", test->name, test->issue);
            test->fn(env);
            tests_run++;

            if (checks_failed != failed_before) {
                tests_failed++;
                printf("  -> %s FAILED (%d check(s))\n",
                       test->name, checks_failed - failed_before);
            }
        }
    }

    printf("\n==== talOS test summary ====\n");
    printf("  tests:  %d run, %d failed\n", tests_run, tests_failed);
    printf("  checks: %d run, %d failed\n", checks_run, checks_failed);
    // This exact string is what scripts/test.sh greps for, so a hang or an
    // early fault can never be mistaken for a pass.
    printf("%s\n", checks_failed == 0 ? "talOS tests: ALL PASSED"
                                      : "talOS tests: FAILURES PRESENT");

    return checks_failed;
}
