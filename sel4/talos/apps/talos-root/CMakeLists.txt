#
# Talos product rootserver.
#
# Thin: links libtalos and sequences it. The musllibc / libsel4 / seL4_libs
# import calls that used to live here are now done once in the top-level
# CMakeLists.txt, because two apps share them.
#
# DeclareRootserver() is likewise called from the top level, which picks
# between this app and apps/talos-tests via -DTALOS_ROOTSERVER.
#
# SPDX-License-Identifier: BSD-2-Clause
#

cmake_minimum_required(VERSION 3.16.0)

file(GLOB talos_root_srcs src/*.c)

add_executable(talos-root EXCLUDE_FROM_ALL ${talos_root_srcs})

target_link_libraries(talos-root PUBLIC talos sel4muslcsys)
target_compile_options(talos-root PRIVATE -Werror -g)
