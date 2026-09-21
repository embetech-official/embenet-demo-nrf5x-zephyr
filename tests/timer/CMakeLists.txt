# SPDX-FileCopyrightText: 2026 Embetech sp. z o.o.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

# This test lives inside the module it exercises, so it points Zephyr at it itself and builds from
# any workspace. See tests/aes128/CMakeLists.txt.
cmake_path(SET EMBENET_MODULE_DIR NORMALIZE "${CMAKE_CURRENT_LIST_DIR}/../..")
list(APPEND ZEPHYR_EXTRA_MODULES ${EMBENET_MODULE_DIR})

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(embenet_timer_test)

target_sources(app PRIVATE src/main.c)

# The timer backend is compiled here rather than pulled in with CONFIG_EMBENET_NODE, because that
# would bring ports/node/src/radio.c with it and radio.c is still nRF52-only (SWI3_EGU3_IRQn,
# NRF_RADIO_SHORT_END_DISABLE_MASK). Reaching into the module for one file keeps this test buildable
# on the nRF54L today; fold it back into the node port once the radio lands.
if (CONFIG_SOC_SERIES_NRF52)
  # The RTC backend guards its counter reads, so it needs the port's critical section too. The GRTC
  # one does not: its counter read is atomic.
  target_sources(app PRIVATE ${EMBENET_MODULE_DIR}/ports/node/src/timer_rtc_nrf52.c
                             ${EMBENET_MODULE_DIR}/ports/node/src/critical_section.c
  )
elseif (CONFIG_SOC_SERIES_NRF54L)
  target_sources(app PRIVATE ${EMBENET_MODULE_DIR}/ports/node/src/timer_grtc_nrf54l.c)
else ()
  message(FATAL_ERROR "no embeNET timer backend for ${CONFIG_SOC_SERIES}")
endif ()

# embenet_port/timer.h ships with the node bundle, which is not searched without CONFIG_EMBENET_NODE.
target_include_directories(app PRIVATE ${EMBENET_MODULE_DIR}/libs/embenet_node/include)
