# SPDX-FileCopyrightText: 2026 Embetech sp. z o.o.
# SPDX-License-Identifier: Apache-2.0

# Resolves the release bundles in libs/ and publishes them to the whole Zephyr build.
#
# Found in dependency order so each bundle's own find_dependency() calls short-circuit instead of
# searching the system. Only the bundles the selected roles need are looked up.
include_guard(GLOBAL)

include(${CMAKE_CURRENT_LIST_DIR}/embenet_arch.cmake)

set(EMBENET_LIBS_DIR ${CMAKE_CURRENT_LIST_DIR}/../libs)

find_package(embeutils REQUIRED PATHS ${EMBENET_LIBS_DIR}/embeutils/cmake NO_DEFAULT_PATH)
find_package(crc REQUIRED PATHS ${EMBENET_LIBS_DIR}/crc/cmake NO_DEFAULT_PATH)
find_package(logger REQUIRED PATHS ${EMBENET_LIBS_DIR}/logger/cmake NO_DEFAULT_PATH)
# The logger bundle's config brings in its imported targets but not its helper commands.
include(${EMBENET_LIBS_DIR}/logger/cmake/logger_utils.cmake)

# logger_set_max_level() matches its CONFIG entries against CMAKE_BUILD_TYPE, and Zephyr builds as
# MinSizeRel - a configuration the ports do not name. Without a fallback, port logging would compile
# out silently.
set(LOGGER_DEFAULT_MAX_LEVEL WARNING CACHE STRING "Logger level for unnamed build configurations")

find_package(ring_buffer REQUIRED PATHS ${EMBENET_LIBS_DIR}/ring_buffer/cmake NO_DEFAULT_PATH)
find_package(embenet_udp REQUIRED PATHS ${EMBENET_LIBS_DIR}/embenet_udp/cmake NO_DEFAULT_PATH)

set(EMBENET_IMPORTED_TARGETS embetech::utils embetech::crc embetech::logger embetech::ring_buffer embenet::udp)

if (CONFIG_EMBENET_NODE)
  find_package(embenet_node REQUIRED PATHS ${EMBENET_LIBS_DIR}/embenet_node/cmake NO_DEFAULT_PATH)
  list(APPEND EMBENET_IMPORTED_TARGETS embenet::node)
endif ()

if (CONFIG_EMBENET_BORDER_ROUTER)
  find_package(embenet_border_router REQUIRED PATHS ${EMBENET_LIBS_DIR}/embenet_border_router/cmake NO_DEFAULT_PATH)
  list(APPEND EMBENET_IMPORTED_TARGETS embenet::border_router)

  # The bundle asks for C++23 as a compile feature, which CMake resolves against
  # CMAKE_CXX_COMPILE_FEATURES - a list Zephyr leaves empty, so even GCC 14 is reported as not
  # supporting it. Same requirement, stated as a flag instead. Build-wide, because an application
  # using the C++ API compiles the same headers.
  set_property(TARGET embenet::border_router-${EMBENET_ARCH} PROPERTY INTERFACE_COMPILE_FEATURES "")
  zephyr_compile_options($<$<COMPILE_LANGUAGE:CXX>:-std=c++23>)
endif ()

# find_package() creates its targets in the calling directory scope only. Publishing them through
# zephyr_interface propagates the headers and puts the archives on the final link line, so an
# application needs no find_package() and no target_link_libraries() of its own.
foreach (embenet_target IN LISTS EMBENET_IMPORTED_TARGETS)
  get_target_property(embenet_target_includes ${embenet_target} INTERFACE_INCLUDE_DIRECTORIES)
  if (embenet_target_includes)
    zephyr_include_directories(${embenet_target_includes})
  endif ()
endforeach ()
unset(embenet_target)
unset(embenet_target_includes)

zephyr_link_libraries(${EMBENET_IMPORTED_TARGETS})
