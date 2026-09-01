#
# libtalostest: the test harness plus one suite per subsystem.
#
# Linked only by apps/talos-tests, never by apps/talos-root, so no test code
# can reach a product image. New suite files under src/ are picked up by the
# glob automatically - see include/talos_test.h for the three-step checklist.
#
# SPDX-License-Identifier: BSD-2-Clause
#

cmake_minimum_required(VERSION 3.16.0)

file(GLOB talos_test_srcs src/*.c)

add_library(talostest STATIC EXCLUDE_FROM_ALL ${talos_test_srcs})

target_include_directories(talostest PUBLIC include)
target_link_libraries(talostest PUBLIC talos)
target_compile_options(talostest PRIVATE -Werror -g)
