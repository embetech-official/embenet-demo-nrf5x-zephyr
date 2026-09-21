# SPDX-FileCopyrightText: 2026 Embetech sp. z o.o.
# SPDX-License-Identifier: Apache-2.0

# Shared by both ports: the queues backing the all-in-one transport, the reset request the root node
# raises for the application, and the semaphore the port ISRs give to wake it.
zephyr_library_named(embenet_port_common)

zephyr_library_sources(src/embenet_port_common.c)
zephyr_include_directories(include)

# Only meaningful once a prebuilt library is linked - it exists to satisfy that library's EXPECT hook.
if (CONFIG_EMBENET_NODE OR CONFIG_EMBENET_BORDER_ROUTER)
  zephyr_library_sources(src/expect_abort.c)
endif ()

# A library reached through a module's own add_subdirectory() does not get this automatically, and
# without it generated headers may not exist yet when these sources compile.
if (TARGET zephyr_generated_headers)
  add_dependencies(embenet_port_common zephyr_generated_headers)
endif ()
