# SPDX-FileCopyrightText: 2026 Embetech sp. z o.o.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

# Lives inside the module it demonstrates, so it points Zephyr at it directly rather than through ZEPHYR_EXTRA_MODULES or a west manifest entry.
cmake_path(SET EMBENET_MODULE_DIR NORMALIZE "${CMAKE_CURRENT_LIST_DIR}/../..")
list(APPEND ZEPHYR_EXTRA_MODULES ${EMBENET_MODULE_DIR})

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(embenet_node_sample)

target_sources(app PRIVATE src/node.c)
