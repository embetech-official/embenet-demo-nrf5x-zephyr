set(embenet_border_router_VERSION 1.1.1)


####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was embenet_border_router-config_template.cmake                            ########

get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../" ABSOLUTE)

macro(set_and_check _var _file)
  set(${_var} "${_file}")
  if(NOT EXISTS "${_file}")
    message(FATAL_ERROR "File or directory ${_file} referenced by variable ${_var} does not exist !")
  endif()
endmacro()

macro(check_required_components _NAME)
  foreach(comp ${${_NAME}_FIND_COMPONENTS})
    if(NOT ${_NAME}_${comp}_FOUND)
      if(${_NAME}_FIND_REQUIRED_${comp})
        set(${_NAME}_FOUND FALSE)
      endif()
    endif()
  endforeach()
endmacro()

####################################################################################

include(CMakeFindDependencyMacro)
find_dependency(embenet_udp)
find_dependency(logger)

include("${CMAKE_CURRENT_LIST_DIR}/architecture_selection.cmake")

if (NOT DEFINED EMBENET_BORDER_ROUTER_ARCHITECTURE)
  detect_architecture_triplet(EMBENET_BORDER_ROUTER_ARCHITECTURE)
  if (NOT EMBENET_BORDER_ROUTER_ARCHITECTURE)
    message(FATAL_ERROR "Unable to auto-detect the target architecture for embenet_border_router. "
                        "Set EMBENET_BORDER_ROUTER_ARCHITECTURE manually before calling find_package(embenet_border_router)."
    )
  endif ()
endif ()

message(DEBUG "EMBENET_BORDER_ROUTER_ARCHITECTURE=${EMBENET_BORDER_ROUTER_ARCHITECTURE}")

find_architecture_targets_file(embenet_border_router "${EMBENET_BORDER_ROUTER_ARCHITECTURE}" "${CMAKE_CURRENT_LIST_DIR}" targets_file)
include("${targets_file}")
unset(targets_file)

add_library(embenet::border_router ALIAS embenet::border_router-${EMBENET_BORDER_ROUTER_ARCHITECTURE})

check_required_components("embenet_border_router")
