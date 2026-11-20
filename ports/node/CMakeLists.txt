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
# name: nRF52 (and the nRF5340 network core, which has the same ECB) hands over one block of RAM
# through ECBDATAPTR, nRF54L keeps the key in registers and moves the data over EasyVDMA.
if (CONFIG_SOC_SERIES_NRF52 OR CONFIG_SOC_NRF5340_CPUNET)
  set(EMBENET_NODE_ECB_SOURCE src/aes128_ecb_nrf52.c)
elseif (CONFIG_SOC_SERIES_NRF54L)
  set(EMBENET_NODE_ECB_SOURCE src/aes128_ecb_nrf54l.c)
else ()
  message(FATAL_ERROR "embeNET Node has no ECB backend for ${CONFIG_SOC_SERIES} - "
                      "EMBENET_AES128_Encrypt() runs in the slot ISR and needs the hardware cipher."
  )
endif ()

# And for the timer behind the slot schedule. The nRF52 owns an RTC instance outright; the nRF54L
# has no RTC at all and borrows one compare channel of the shared GRTC. The nRF5340 network core has
# only RTC0 and RTC1 - MPSL's and the system clock's - so it borrows a compare channel of RTC1.
if (CONFIG_SOC_SERIES_NRF52)
  set(EMBENET_NODE_TIMER_SOURCE src/timer_rtc_nrf52.c)
elseif (CONFIG_SOC_NRF5340_CPUNET)
  set(EMBENET_NODE_TIMER_SOURCE src/timer_rtc_nrf53.c)
elseif (CONFIG_SOC_SERIES_NRF54L)
  set(EMBENET_NODE_TIMER_SOURCE src/timer_grtc_nrf54l.c)
else ()
  message(FATAL_ERROR "embeNET Node has no timer backend for ${CONFIG_SOC_SERIES} - "
                      "the stack cannot schedule slots without one."
  )
endif ()

# And for the radio. The two families diverge too far for one file: PPI endpoint pairs against DPPI
# publish/subscribe, END against PHYEND for the end-of-frame shortcut, a POWER register on one side
# and none on the other. The nRF5340 network core has the nRF52 RADIO behind DPPI, which
# radio_nrf52.c covers with a few conditionals.
if (CONFIG_SOC_SERIES_NRF52 OR CONFIG_SOC_NRF5340_CPUNET)
  set(EMBENET_NODE_RADIO_SOURCE src/radio_nrf52.c)
elseif (CONFIG_SOC_SERIES_NRF54L)
  set(EMBENET_NODE_RADIO_SOURCE src/radio_nrf54l.c)
else ()
  message(FATAL_ERROR "embeNET Node has no radio backend for ${CONFIG_SOC_SERIES}.")
endif ()

zephyr_library_include_directories(src/include_private)
# The port's public API for applications: embenet_port_node/multiprotocol_stats.h.
zephyr_include_directories(include)

zephyr_library_sources(
  ${EMBENET_NODE_BRT_SOURCE}
  ${EMBENET_NODE_ECB_SOURCE}
  ${EMBENET_NODE_RADIO_SOURCE}
  ${EMBENET_NODE_TIMER_SOURCE}
  src/aes128.c
  src/allocator.c
  src/api_version.c
  src/critical_section.c
  src/eui64.c
  src/random.c
)

# The adapter that lets another protocol stack share the transceiver. Off by default: it needs MPSL,
# and a node that owns the radio outright has no use for it. Without the adapter nothing calls the
# stack's EMBENET_MULTIPROTOCOL_OnGranted()/OnEndGuard(), so the stack's multiprotocol support is not
# linked in and the stack owns the transceiver.
if (CONFIG_EMBENET_NODE_MULTIPROTOCOL)
  zephyr_library_sources(src/multiprotocol.c src/multiprotocol_port.c)
endif ()

# A Bluetooth controller in the image is a second owner of the same transceiver. Without the arbiter
# the two drive RADIO against each other, which on hardware looks like a node that transmits into
# other people's events and misses its own - a much harder thing to recognize than this message.
if (CONFIG_BT AND NOT CONFIG_EMBENET_NODE_MULTIPROTOCOL)
  message(FATAL_ERROR "CONFIG_BT and the embeNET Node both drive the radio. Enable "
                      "CONFIG_EMBENET_NODE_MULTIPROTOCOL so they arbitrate for it through MPSL."
  )
endif ()

# No target_link_libraries() for the stack: its headers and archive arrive through zephyr_interface
# (see cmake/embenet_libs.cmake), which puts the archive after the Zephyr libraries on the link line.
# That order matters with CONFIG_EMBENET_NODE_MULTIPROTOCOL: the adapter calls the stack back, so the
# stack has to come after this port for those references to pull its multiprotocol module in.

# ocrypto, the software cipher behind EMBENET_AES128_Decrypt(). Pulled in by CONFIG_NRF_OBERON.
zephyr_library_link_libraries(nrfxlib_crypto)

# See ports/common/CMakeLists.txt.
if (TARGET zephyr_generated_headers)
  add_dependencies(embenet_node_port zephyr_generated_headers)
endif ()
