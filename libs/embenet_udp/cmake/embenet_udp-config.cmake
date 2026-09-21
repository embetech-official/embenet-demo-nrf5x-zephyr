
####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was embenet_udp-config_template.cmake                            ########

get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../" ABSOLUTE)

macro(set_and_check _var _file)
  set(${_var} "${_file}")
  if(NOT EXISTS "${_file}")
    message(FATAL_ERROR "File or directory ${_file} referenced by variable ${_var} does not exist !")
  endif()
endmacro()

####################################################################################

# 'udp' (public C API) is always required — load it first
include("${CMAKE_CURRENT_LIST_DIR}/embenet_udp_udp-targets.cmake")

# Discover optional installed components (port_*, mock, ...)
file(GLOB embenet_udp_found_components RELATIVE "${CMAKE_CURRENT_LIST_DIR}" "${CMAKE_CURRENT_LIST_DIR}/embenet_udp_*-targets.cmake")
list(TRANSFORM embenet_udp_found_components REPLACE "embenet_udp_(.*)-targets\\.cmake" "\\1")
list(REMOVE_ITEM embenet_udp_found_components udp)
message(DEBUG "embenet_udp: optional components available: ${embenet_udp_found_components}")

if (embenet_udp_FIND_COMPONENTS)
  set(embenet_udp_requested_components ${embenet_udp_FIND_COMPONENTS})
  list(REMOVE_ITEM embenet_udp_requested_components udp)
else ()
  set(embenet_udp_requested_components ${embenet_udp_found_components})
endif ()

foreach (component ${embenet_udp_requested_components})
  if (NOT component IN_LIST embenet_udp_found_components)
    set(embenet_udp_FOUND FALSE)
    set(embenet_udp_NOT_FOUND_MESSAGE "Requested component not installed: ${component}")
    return()
  endif ()
  include("${CMAKE_CURRENT_LIST_DIR}/embenet_udp_${component}-targets.cmake")
endforeach ()
