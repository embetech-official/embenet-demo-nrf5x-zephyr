# SPDX-FileCopyrightText: 2026 Embetech sp. z o.o.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

# Adds this repository as a Zephyr module, so the sample builds without a west manifest entry.
cmake_path(SET EMBENET_MODULE_DIR NORMALIZE "${CMAKE_CURRENT_LIST_DIR}/../..")
list(APPEND ZEPHYR_EXTRA_MODULES ${EMBENET_MODULE_DIR})

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(embenet_border_router_integrated_sample)

# The root node and the border router together need more than the 64 KB of RAM these have; the link would fail on it.
if (CONFIG_SOC_NRF52832 OR CONFIG_SOC_NRF5340_CPUNET)
  message(FATAL_ERROR "border_router_integrated does not fit ${CONFIG_SOC}: it needs more than 64 KB of RAM. "
                      "Use an nRF54L15, nRF52840 or nRF52833 development kit.")
endif ()

target_sources(app PRIVATE src/main.c src/root.c src/border_router.c)

set(COMMON_DIR ${CMAKE_CURRENT_LIST_DIR}/../common)
target_sources(app PRIVATE ${COMMON_DIR}/blinky_service/blinky_io.c ${COMMON_DIR}/blinky_service/blinky_service_common.c
                           ${COMMON_DIR}/blinky_service/blinky_service_router.c
)
target_include_directories(app PRIVATE ${COMMON_DIR} ${COMMON_DIR}/blinky_service)
