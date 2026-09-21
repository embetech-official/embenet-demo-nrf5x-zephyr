# SPDX-FileCopyrightText: 2026 Embetech sp. z o.o.
# SPDX-License-Identifier: Apache-2.0

zephyr_library_named(embenet_node_port)

# Compile-time choice: exactly one implementation defines the EMBENET_BRT_* symbols.
if (CONFIG_EMBENET_TRANSPORT_INMEM)
  set(EMBENET_NODE_BRT_SOURCE src/brt_inmem.c)
else ()
  set(EMBENET_NODE_BRT_SOURCE src/brt_uart.c)
endif ()

# Same idea for the ECB backend behind aes128.c. The two families share nothing but the peripheral
# name: nRF52 hands over one block of RAM through ECBDATAPTR, nRF54L keeps the key in registers and
# moves the data over EasyVDMA.
if (CONFIG_SOC_SERIES_NRF52)
  set(EMBENET_NODE_ECB_SOURCE src/aes128_ecb_nrf52.c)
elseif (CONFIG_SOC_SERIES_NRF54L)
  set(EMBENET_NODE_ECB_SOURCE src/aes128_ecb_nrf54l.c)
else ()
  message(FATAL_ERROR "embeNET Node has no ECB backend for ${CONFIG_SOC_SERIES} - "
                      "EMBENET_AES128_Encrypt() runs in the slot ISR and needs the hardware cipher."
  )
endif ()

# And for the timer behind the slot schedule. The nRF52 owns an RTC instance outright; the nRF54L
# has no RTC at all and borrows one compare channel of the shared GRTC.
if (CONFIG_SOC_SERIES_NRF52)
  set(EMBENET_NODE_TIMER_SOURCE src/timer_rtc_nrf52.c)
elseif (CONFIG_SOC_SERIES_NRF54L)
  set(EMBENET_NODE_TIMER_SOURCE src/timer_grtc_nrf54l.c)
else ()
  message(FATAL_ERROR "embeNET Node has no timer backend for ${CONFIG_SOC_SERIES} - "
                      "the stack cannot schedule slots without one."
  )
endif ()

# And for the radio. The two families diverge too far for one file: PPI endpoint pairs against DPPI
# publish/subscribe, END against PHYEND for the end-of-frame shortcut, a POWER register on one side
# and none on the other.
if (CONFIG_SOC_SERIES_NRF52)
  set(EMBENET_NODE_RADIO_SOURCE src/radio_nrf52.c)
elseif (CONFIG_SOC_SERIES_NRF54L)
  set(EMBENET_NODE_RADIO_SOURCE src/radio_nrf54l.c)
else ()
  message(FATAL_ERROR "embeNET Node has no radio backend for ${CONFIG_SOC_SERIES}.")
endif ()

zephyr_library_include_directories(src/include_private)

zephyr_library_sources(
  ${EMBENET_NODE_BRT_SOURCE}
  ${EMBENET_NODE_ECB_SOURCE}
  ${EMBENET_NODE_RADIO_SOURCE}
  ${EMBENET_NODE_TIMER_SOURCE}
  src/aes128.c
  src/allocator.c
  src/capabilities.c
  src/critical_section.c
  src/eui64.c
  src/random.c
)

# No target_link_libraries() for the stack: the dependency runs the other way round - the library
# calls into this port. Its headers arrive through zephyr_interface (see cmake/embenet_libs.cmake).

# ocrypto, the software cipher behind EMBENET_AES128_Decrypt(). Pulled in by CONFIG_NRF_OBERON.
zephyr_library_link_libraries(nrfxlib_crypto)

# See ports/common/CMakeLists.txt.
if (TARGET zephyr_generated_headers)
  add_dependencies(embenet_node_port zephyr_generated_headers)
endif ()
