# SPDX-FileCopyrightText: 2026 Embetech sp. z o.o.
# SPDX-License-Identifier: Apache-2.0

# Kconfig ignores select on choice entries, so the role cannot pull libstdc++ in itself. Caught here
# rather than at the link, where it surfaces as unresolved std:: symbols.
if (NOT CONFIG_GLIBCXX_LIBCPP)
  message(FATAL_ERROR "embeNET Border Router requires CONFIG_GLIBCXX_LIBCPP=y in the application's "
                      "prj.conf - the library uses std::pmr, std::error_category and exceptions."
  )
endif ()

zephyr_library_named(embenet_border_router_port)

# Compile-time choice: exactly one implementation defines the EMBENET_BR_PORT_RootNodeConnector*
# symbols.
if (CONFIG_EMBENET_TRANSPORT_INMEM)
  set(EMBENET_BR_ROOT_NODE_CONNECTOR src/root_node_connector_inmem.cpp)
else ()
  set(EMBENET_BR_ROOT_NODE_CONNECTOR src/root_node_connector_uart.cpp)
endif ()

zephyr_library_sources(
  ${EMBENET_BR_ROOT_NODE_CONNECTOR}
  src/clock.cpp
  src/condition_variable.cpp
  src/memory.cpp
  src/mutex.cpp
  src/network_adapter.c
  src/random.cpp
)

target_include_directories(embenet_border_router_port PRIVATE src/include_private)

# No target_link_libraries() for the stack: the dependency runs the other way round - the library
# calls into this port. Its headers arrive through zephyr_interface (see cmake/embenet_libs.cmake).

# The port logs through Zephyr, not through embetech's logger - CONFIG_EMBENET_LOG_LEVEL gates it,
# the same knob the node port uses. The bundled libraries keep their own logger channels.

# See ports/common/CMakeLists.txt.
if (TARGET zephyr_generated_headers)
  add_dependencies(embenet_border_router_port zephyr_generated_headers)
endif ()
