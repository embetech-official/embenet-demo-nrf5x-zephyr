# SPDX-FileCopyrightText: 2026 Embetech sp. z o.o.
# SPDX-License-Identifier: Apache-2.0

if (NOT CONFIG_EMBENET)
  return()
endif ()

if ((NOT CONFIG_EMBENET_NODE) AND (NOT CONFIG_EMBENET_BORDER_ROUTER))
  message(WARNING "CONFIG_EMBENET is set but no role is selected - " "enable CONFIG_EMBENET_NODE and/or CONFIG_EMBENET_BORDER_ROUTER.")
endif ()

# Make embeNET stack libraries available. The bundles are Cortex-M only, so resolving them is tied to
# a role being selected - that is what lets a role-less build (ports/common and its tests) target the
# host.
if (CONFIG_EMBENET_NODE OR CONFIG_EMBENET_BORDER_ROUTER)
  include(${CMAKE_CURRENT_LIST_DIR}/cmake/embenet_libs.cmake)
endif ()

# Add ports sources
add_subdirectory(ports/common)

if (CONFIG_EMBENET_NODE)
  add_subdirectory(ports/node)
endif ()

if (CONFIG_EMBENET_BORDER_ROUTER)
  add_subdirectory(ports/border_router)
endif ()
