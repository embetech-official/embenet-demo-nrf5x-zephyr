# SPDX-FileCopyrightText: 2026 Embetech sp. z o.o.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

# Adds this repository as a Zephyr module, so the sample builds without a west manifest entry.
cmake_path(SET EMBENET_MODULE_DIR NORMALIZE "${CMAKE_CURRENT_LIST_DIR}/../..")
list(APPEND ZEPHYR_EXTRA_MODULES ${EMBENET_MODULE_DIR})

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(embenet_node_ble_sample)

target_sources(app PRIVATE src/node.c)

set(COMMON_DIR ${CMAKE_CURRENT_LIST_DIR}/../common)
target_sources(app PRIVATE ${COMMON_DIR}/blinky_service/blinky_io.c ${COMMON_DIR}/blinky_service/blinky_service_common.c
                           ${COMMON_DIR}/blinky_service/blinky_service_node.c ${COMMON_DIR}/time_report/time_report.c
)
target_include_directories(app PRIVATE ${COMMON_DIR} ${COMMON_DIR}/blinky_service ${COMMON_DIR}/time_report)

# The embeNET middleware and its GATT services.
target_sources(app PRIVATE src/embenet.c src/embenet_ble.c src/embenet_ble_provisioning.c src/embenet_ble_info.c)
