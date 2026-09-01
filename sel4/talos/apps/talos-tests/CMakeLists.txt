#
# Talos test rootserver. Built and declared as the rootserver only when
# configured with -DTALOS_ROOTSERVER=tests.
#
# SPDX-License-Identifier: BSD-2-Clause
#

cmake_minimum_required(VERSION 3.16.0)

file(GLOB talos_tests_srcs src/*.c)

add_executable(talos-tests EXCLUDE_FROM_ALL ${talos_tests_srcs})

target_link_libraries(talos-tests PUBLIC talos talostest sel4muslcsys)
target_compile_options(talos-tests PRIVATE -Werror -g)
